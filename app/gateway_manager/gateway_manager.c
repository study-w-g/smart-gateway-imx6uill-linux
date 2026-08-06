#define _GNU_SOURCE

#include "mqtt_client.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "../../drivers/gpio_event/gpio_event_uapi.h"

#define DS18B20_DEVICE "/dev/ds18b20"
#define SHT30_DEVICE   "/dev/sht30"
#define GPIO_DEVICE    "/dev/gpio-event"
#define LOCAL_SOCKET    "/tmp/smart-gateway.sock"
#define CONFIG_FILE     "configs/mqtt.conf"
#define SAMPLE_PERIOD_S 2
#define TEMP_HIGH       30.0f

struct sht30_measurement {
	uint16_t raw_temperature;
	uint16_t raw_humidity;
};

struct gateway_state {
	float ds18b20_temperature;
	float sht30_temperature;
	float sht30_humidity;
	int ds18b20_valid;
	int sht30_valid;
	int led_value;
	unsigned long sample_sequence;
	char last_event[128];
	int pending_led_valid;
	int pending_led_value;
};

static volatile sig_atomic_t running = 1;
static pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
static struct gateway_state state;

static void set_led(int fd, int value);

static void mqtt_command_received(const char *topic, const char *payload,
					  void *userdata)
{
	struct gateway_state *gateway = userdata;
	int value;
	const char *led_field;

	(void)topic;
	if (!payload || !gateway)
		return;

	/* 为便于学习，同时支持“led 0”和 {"led":1} 两种命令格式。 */
	if (sscanf(payload, "led %d", &value) != 1) {
		led_field = strstr(payload, "\"led\"");
		if (!led_field ||
		    sscanf(led_field, "\"led\"%*[: ]%d", &value) != 1)
			return;
	}
	if (value != 0 && value != 1)
		return;

	pthread_mutex_lock(&state_lock);
	gateway->pending_led_value = value;
	gateway->pending_led_valid = 1;
	pthread_mutex_unlock(&state_lock);
}

static void apply_pending_mqtt_command(int gpio_fd)
{
	int value;

	pthread_mutex_lock(&state_lock);
	if (!state.pending_led_valid) {
		pthread_mutex_unlock(&state_lock);
		return;
	}
	value = state.pending_led_value;
	state.pending_led_valid = 0;
	pthread_mutex_unlock(&state_lock);
	set_led(gpio_fd, value);
}

static void stop_gateway(int signal_number)
{
	(void)signal_number;
	running = 0;
}

static float ds18b20_raw_to_celsius(int16_t raw)
{
	/* DS18B20 12 位分辨率下，最低位代表 1/16 摄氏度。 */
	return (float)raw / 16.0f;
}

static float sht30_raw_to_celsius(uint16_t raw)
{
	return -45.0f + 175.0f * (float)raw / 65535.0f;
}

static float sht30_raw_to_humidity(uint16_t raw)
{
	return 100.0f * (float)raw / 65535.0f;
}

static int read_ds18b20(void)
{
	int fd;
	int16_t raw;
	ssize_t length;

	fd = open(DS18B20_DEVICE, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		pthread_mutex_lock(&state_lock);
		state.ds18b20_valid = 0;
		pthread_mutex_unlock(&state_lock);
		return -errno;
	}
	length = read(fd, &raw, sizeof(raw));
	close(fd);
	if (length != sizeof(raw)) {
		pthread_mutex_lock(&state_lock);
		state.ds18b20_valid = 0;
		pthread_mutex_unlock(&state_lock);
		return length < 0 ? -errno : -EIO;
	}

	pthread_mutex_lock(&state_lock);
	state.ds18b20_temperature = ds18b20_raw_to_celsius(raw);
	state.ds18b20_valid = 1;
	pthread_mutex_unlock(&state_lock);
	return 0;
}

static int read_sht30(void)
{
	int fd;
	struct sht30_measurement measurement;
	ssize_t length;

	fd = open(SHT30_DEVICE, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		pthread_mutex_lock(&state_lock);
		state.sht30_valid = 0;
		pthread_mutex_unlock(&state_lock);
		return -errno;
	}
	length = read(fd, &measurement, sizeof(measurement));
	close(fd);
	if (length != sizeof(measurement)) {
		pthread_mutex_lock(&state_lock);
		state.sht30_valid = 0;
		pthread_mutex_unlock(&state_lock);
		return length < 0 ? -errno : -EIO;
	}

	pthread_mutex_lock(&state_lock);
	state.sht30_temperature =
		sht30_raw_to_celsius(measurement.raw_temperature);
	state.sht30_humidity = sht30_raw_to_humidity(measurement.raw_humidity);
	state.sht30_valid = 1;
	pthread_mutex_unlock(&state_lock);
	return 0;
}

static void set_led(int fd, int value)
{
	uint32_t led = value ? 1 : 0;

	/* ioctl 编号必须与 gpio_event 驱动保持一致。 */
	(void)ioctl(fd, GPIO_EVENT_IOC_SET_LED, &led);
	pthread_mutex_lock(&state_lock);
	state.led_value = led;
	pthread_mutex_unlock(&state_lock);
}

static void apply_temperature_alarm(int gpio_fd)
{
	struct gateway_state snapshot;
	int alarm;

	pthread_mutex_lock(&state_lock);
	snapshot = state;
	pthread_mutex_unlock(&state_lock);

	/* 仅在 SHT30 数据有效时判断阈值，避免无效值触发误报警。 */
	alarm = snapshot.sht30_valid &&
		(snapshot.sht30_temperature >= TEMP_HIGH);
	set_led(gpio_fd, alarm);
}

static void handle_gpio_events(int fd)
{
	struct gpio_event_record event;
	ssize_t length;

	length = read(fd, &event, sizeof(event));
	if (length != sizeof(event))
		return;

	pthread_mutex_lock(&state_lock);
	snprintf(state.last_event, sizeof(state.last_event),
		 "按键事件：电平=%u，序号=%u", event.value, event.sequence);
	pthread_mutex_unlock(&state_lock);

	/* 示例业务：按键按下时翻转 LED。实际有效电平需按硬件修改。 */
	pthread_mutex_lock(&state_lock);
	int next_led = !state.led_value;
	pthread_mutex_unlock(&state_lock);
	set_led(fd, next_led);
}

static void state_to_json(char *buffer, size_t size)
{
	struct gateway_state snapshot;

	pthread_mutex_lock(&state_lock);
	snapshot = state;
	pthread_mutex_unlock(&state_lock);

	snprintf(buffer, size,
		 "{\"ds18b20_temperature\":%.2f,"
		 "\"sht30_temperature\":%.2f,"
		 "\"sht30_humidity\":%.2f,"
		 "\"ds18b20_valid\":%d,\"sht30_valid\":%d,"
		 "\"led\":%d,\"sequence\":%lu,\"event\":\"%s\"}",
		 snapshot.ds18b20_temperature, snapshot.sht30_temperature,
		 snapshot.sht30_humidity, snapshot.ds18b20_valid,
		 snapshot.sht30_valid, snapshot.led_value,
		 snapshot.sample_sequence, snapshot.last_event);
}

static int create_local_socket(void)
{
	struct sockaddr_un address;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return -1;
	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, LOCAL_SOCKET, sizeof(address.sun_path) - 1);
	unlink(LOCAL_SOCKET);
	if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0 ||
	    listen(fd, 4) < 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static void serve_local_client(int client_fd, int gpio_fd)
{
	char command[128];
	char response[1024];
	ssize_t length;

	length = read(client_fd, command, sizeof(command) - 1);
	if (length <= 0)
		return;
	command[length] = '\0';

	if (strncmp(command, "led ", 4) == 0) {
		int value = atoi(command + 4);
		set_led(gpio_fd, value);
		strcpy(response, "{\"result\":\"ok\"}\n");
	} else if (strncmp(command, "status", 6) == 0) {
		state_to_json(response, sizeof(response));
		strcat(response, "\n");
	} else {
		strcpy(response, "{\"result\":\"unknown command\"}\n");
	}
	(void)write(client_fd, response, strlen(response));
}

static void publish_state(struct mqtt_client *mqtt,
				  const struct mqtt_config *config)
{
	char payload[1024];

	state_to_json(payload, sizeof(payload));
	if (mqtt)
		(void)mqtt_client_publish(mqtt, config->topic_telemetry, payload);
}

int main(int argc, char **argv)
{
	struct mqtt_config mqtt_config;
	struct mqtt_client *mqtt = NULL;
	struct pollfd poll_fds[3];
	int local_fd;
	int gpio_fd;
	int ret;
	(void)argc;
	(void)argv;

	signal(SIGINT, stop_gateway);
	signal(SIGTERM, stop_gateway);
	memset(&state, 0, sizeof(state));

	ret = mqtt_config_load(CONFIG_FILE, &mqtt_config);
	if (ret == 0)
		mqtt = mqtt_client_create(&mqtt_config,
					  mqtt_command_received, &state);

	local_fd = create_local_socket();
	gpio_fd = open(GPIO_DEVICE, O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (local_fd < 0 || gpio_fd < 0) {
		fprintf(stderr, "请先加载 GPIO 驱动并检查 /dev/gpio-event\n");
		if (local_fd >= 0)
			close(local_fd);
		if (gpio_fd >= 0)
			close(gpio_fd);
		mqtt_client_destroy(mqtt);
		return 1;
	}

	while (running) {
		int client_fd;

		(void)read_ds18b20();
		(void)read_sht30();
		apply_temperature_alarm(gpio_fd);
		pthread_mutex_lock(&state_lock);
		state.sample_sequence++;
		pthread_mutex_unlock(&state_lock);

		publish_state(mqtt, &mqtt_config);
		if (mqtt)
			(void)mqtt_client_loop(mqtt, 10);
		apply_pending_mqtt_command(gpio_fd);

		poll_fds[0].fd = local_fd;
		poll_fds[0].events = POLLIN;
		poll_fds[1].fd = gpio_fd;
		poll_fds[1].events = POLLIN;
		poll_fds[2].fd = -1;
		poll_fds[2].events = 0;
		ret = poll(poll_fds, 2, SAMPLE_PERIOD_S * 1000);
		if (ret < 0 && errno == EINTR)
			continue;
		if (ret > 0 && (poll_fds[1].revents & POLLIN))
			handle_gpio_events(gpio_fd);
		if (ret > 0 && (poll_fds[0].revents & POLLIN)) {
			client_fd = accept4(local_fd, NULL, NULL, SOCK_CLOEXEC);
			if (client_fd >= 0) {
				serve_local_client(client_fd, gpio_fd);
				close(client_fd);
			}
		}
	}

	close(gpio_fd);
	close(local_fd);
	unlink(LOCAL_SOCKET);
	mqtt_client_destroy(mqtt);
	return 0;
}
