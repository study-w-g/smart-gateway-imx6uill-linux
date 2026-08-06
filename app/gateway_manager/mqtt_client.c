#include "mqtt_client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * 这里使用 Eclipse Mosquitto 的 C 客户端库。
 * Linux 目标系统需要安装 libmosquitto 的头文件和运行库。
 */
#include <mosquitto.h>

struct mqtt_client {
	struct mosquitto *mosq;
	struct mqtt_config config;
	mqtt_command_callback callback;
	void *userdata;
	time_t next_reconnect;
	bool connected;
};

static void mqtt_message_callback(struct mosquitto *mosq, void *userdata,
					  const struct mosquitto_message *message)
{
	struct mqtt_client *client = userdata;
	char payload[256];
	int length;

	(void)mosq;
	if (client->callback && message && message->payload)
	{
		length = message->payloadlen;
		if (length >= (int)sizeof(payload))
			length = sizeof(payload) - 1;
		memcpy(payload, message->payload, length);
		payload[length] = '\0';
		client->callback(message->topic, payload, client->userdata);
	}
}

static int mqtt_connect_and_subscribe(struct mqtt_client *client)
{
	int ret;

	ret = mosquitto_connect(client->mosq, client->config.broker,
				client->config.port, client->config.keepalive);
	if (ret != MOSQ_ERR_SUCCESS) {
		client->connected = false;
		client->next_reconnect = time(NULL) + 5;
		return -EIO;
	}

	ret = mosquitto_subscribe(client->mosq, NULL,
					client->config.topic_command,
					client->config.qos);
	if (ret != MOSQ_ERR_SUCCESS) {
		mosquitto_disconnect(client->mosq);
		client->connected = false;
		client->next_reconnect = time(NULL) + 5;
		return -EIO;
	}
	client->connected = true;
	return 0;
}

static void trim_line(char *line)
{
	char *start = line;
	char *end;

	while (*line == ' ' || *line == '\t')
		line++;
	if (line != start)
		memmove(start, line, strlen(line) + 1);
	line = start;
	end = line + strlen(line);
	while (end > line && (end[-1] == '\n' || end[-1] == '\r' ||
				      end[-1] == ' ' || end[-1] == '\t'))
		*--end = '\0';
}

static int config_value(char *line, const char *key, char **value)
{
	size_t length = strlen(key);

	if (strncmp(line, key, length) != 0 || line[length] != '=')
		return 0;
	*value = line + length + 1;
	return 1;
}

int mqtt_config_load(const char *path, struct mqtt_config *config)
{
	FILE *file;
	char line[256];
	char *value;

	memset(config, 0, sizeof(*config));
	config->port = 1883;
	config->keepalive = 60;
	config->qos = 1;

	file = fopen(path, "r");
	if (!file)
		return -errno;

	while (fgets(line, sizeof(line), file)) {
		trim_line(line);
		if (line[0] == '\0' || line[0] == '#')
			continue;
		if (config_value(line, "broker", &value))
			strncpy(config->broker, value, sizeof(config->broker) - 1);
		else if (config_value(line, "port", &value))
			config->port = atoi(value);
		else if (config_value(line, "client_id", &value))
			strncpy(config->client_id, value, sizeof(config->client_id) - 1);
		else if (config_value(line, "username", &value))
			strncpy(config->username, value, sizeof(config->username) - 1);
		else if (config_value(line, "password", &value))
			strncpy(config->password, value, sizeof(config->password) - 1);
		else if (config_value(line, "keepalive", &value))
			config->keepalive = atoi(value);
		else if (config_value(line, "qos", &value))
			config->qos = atoi(value);
		else if (config_value(line, "topic_telemetry", &value))
			strncpy(config->topic_telemetry, value,
				sizeof(config->topic_telemetry) - 1);
		else if (config_value(line, "topic_event", &value))
			strncpy(config->topic_event, value,
				sizeof(config->topic_event) - 1);
		else if (config_value(line, "topic_command", &value))
			strncpy(config->topic_command, value,
				sizeof(config->topic_command) - 1);
		else if (config_value(line, "topic_response", &value))
			strncpy(config->topic_response, value,
				sizeof(config->topic_response) - 1);
	}
	fclose(file);

	if (config->broker[0] == '\0' || config->client_id[0] == '\0')
		return -EINVAL;
	return 0;
}

struct mqtt_client *mqtt_client_create(const struct mqtt_config *config,
					       mqtt_command_callback callback,
					       void *userdata)
{
	struct mqtt_client *client;
	int ret;

	client = calloc(1, sizeof(*client));
	if (!client)
		return NULL;
	client->config = *config;
	client->callback = callback;
	client->userdata = userdata;

	mosquitto_lib_init();
	client->mosq = mosquitto_new(config->client_id, true, client);
	if (!client->mosq)
		goto failed;
	mosquitto_message_callback_set(client->mosq, mqtt_message_callback);

	if (config->username[0] != '\0') {
		ret = mosquitto_username_pw_set(client->mosq, config->username,
						config->password[0] ? config->password : NULL);
		if (ret != MOSQ_ERR_SUCCESS)
			goto failed;
	}

	/* 首次连接失败时保留本地网关，loop() 中会定时重连。 */
	(void)mqtt_connect_and_subscribe(client);
	return client;

failed:
	if (client->mosq)
		mosquitto_destroy(client->mosq);
	mosquitto_lib_cleanup();
	free(client);
	return NULL;
}

void mqtt_client_destroy(struct mqtt_client *client)
{
	if (!client)
		return;
	if (client->connected)
		mosquitto_disconnect(client->mosq);
	mosquitto_destroy(client->mosq);
	mosquitto_lib_cleanup();
	free(client);
}

int mqtt_client_publish(struct mqtt_client *client, const char *topic,
				const char *payload)
{
	int ret;

	if (!client || !client->connected)
		return -ENOTCONN;
	ret = mosquitto_publish(client->mosq, NULL, topic, (int)strlen(payload),
				       payload, client->config.qos, false);
	return ret == MOSQ_ERR_SUCCESS ? 0 : -EIO;
}

int mqtt_client_loop(struct mqtt_client *client, int timeout_ms)
{
	int ret;

	if (!client)
		return -ENOTCONN;
	if (!client->connected) {
		if (time(NULL) >= client->next_reconnect)
			(void)mqtt_connect_and_subscribe(client);
		return 0;
	}
	ret = mosquitto_loop(client->mosq, timeout_ms, 1);
	if (ret != MOSQ_ERR_SUCCESS) {
		client->connected = false;
		client->next_reconnect = time(NULL) + 5;
		return -EIO;
	}
	return 0;
}
