#include "app.h"
#include "platform.h"

int app_run(const app_config_t *config, int argc, char *argv[]) {
    return platform_app_run(config, argc, argv);
}
void app_emit(const char *event, const char *json_data) {
    platform_app_emit(event, json_data);
}