#ifndef SMART_GATEWAY_MQTT_CLIENT_H
#define SMART_GATEWAY_MQTT_CLIENT_H

#include <stdbool.h>

struct mqtt_client;

typedef void (*mqtt_command_callback)(const char *topic,
					      const char *payload,
					      void *userdata);

/* MQTT 配置从 configs/mqtt.conf 读取。 */
struct mqtt_config {
	char broker[128];
	int port;
	char client_id[128];
	char username[128];
	char password[128];
	int keepalive;
	int qos;
	char topic_telemetry[160];
	char topic_event[160];
	char topic_command[160];
	char topic_response[160];
};

int mqtt_config_load(const char *path, struct mqtt_config *config);
struct mqtt_client *mqtt_client_create(const struct mqtt_config *config,
					       mqtt_command_callback callback,
					       void *userdata);
void mqtt_client_destroy(struct mqtt_client *client);
int mqtt_client_publish(struct mqtt_client *client, const char *topic,
				const char *payload);
int mqtt_client_loop(struct mqtt_client *client, int timeout_ms);

#endif
