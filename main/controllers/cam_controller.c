#include "controllers/cam_controller.h"

static const char* TAG = "cam_controller";

esp_err_t init_camera(void) {
    // initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

picture_data_t take_picture(void) {
    picture_data_t pic_data = {0};

    // makes sure previous picture is deleted
    pic_data.pic = esp_camera_fb_get();
    if (pic_data.pic) {
        free_picture_data(&pic_data);  // delete previous picture
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);

    pic_data.pic = esp_camera_fb_get();
    if (!pic_data.pic) {
        ESP_LOGE(TAG, "Falha ao capturar imagem (fb_get)");
        return pic_data;
    }

    return pic_data;
}

void free_picture_data(picture_data_t* pic_data) {
    if (pic_data->base64_buf) {
        free(pic_data->base64_buf);
        pic_data->base64_buf = NULL;
    }
    if (pic_data->pic) {
        esp_camera_fb_return(pic_data->pic);
        pic_data->pic = NULL;
    }
}