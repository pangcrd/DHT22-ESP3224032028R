#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <DHT.h>  
#include <lvgl.h>
#include "ui.h"
#include <hal/lv_hal_disp.h>


/**************************DTH************************/
#define DHTPIN 27
#define DHTTYPE    DHT22   
DHT dht(DHTPIN, DHTTYPE);

/**************************DTH END************************/
#define XPT2046_IRQ 36 //GPIO driver cảm ứng 
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240,touchScreenMaximumY = 3800; //Chạy Calibration để lấy giá trị mỗi màn hình mỗi khác

static const uint16_t screenWidth  = 320; //Cài độ phân giải màn hình
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp_drv );
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_drv, lv_indev_data_t * data )
{
    if(ts.touched())
    {
        TS_Point p = ts.getPoint();
        //Some very basic auto calibration so it doesn't go out of range
        if(p.x < touchScreenMinimumX) touchScreenMinimumX = p.x;
        if(p.x > touchScreenMaximumX) touchScreenMaximumX = p.x;
        if(p.y < touchScreenMinimumY) touchScreenMinimumY = p.y;
        if(p.y > touchScreenMaximumY) touchScreenMaximumY = p.y;
        //Map this to the pixel position
        data->point.x = map(p.x,touchScreenMinimumX,touchScreenMaximumX,1,screenWidth); /* Touchscreen X calibration */
        data->point.y = map(p.y,touchScreenMinimumY,touchScreenMaximumY,1,screenHeight); /* Touchscreen Y calibration */
        data->state = LV_INDEV_STATE_PR;

        // Serial.print( "Touch x " );
        // Serial.print( data->point.x );
        // Serial.print( " y " );
        // Serial.println( data->point.y );
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}
/**************************Đẩy giá trị nhiệt độ/độ ẩm từ cảm biến DTH22 lên màn hình************************/
void tempHumi(){
  /*Nhiệt độ*/
  float temperature = dht.readTemperature();
   // Chuyển đổi giá trị float thành chuỗi ký tự
  String tempString = String(temperature, 1)+ " *C";  // Lấy 1 chữ số thập phân
  const char* tempValue = tempString.c_str();   // Chuyển đổi từ String sang const char*
  lv_label_set_text(ui_temperture, tempValue);   // Cập nhật nhãn LVGL
  /*Độ ẩm*/
  float humidity = dht.readHumidity();
  String humidString = String(humidity, 1)+ " %";
  const char* humiValue = humidString.c_str();  
  lv_label_set_text(ui_Humi, humiValue);

}

void my_timer(lv_timer_t * timer) { //Đẩy giá trị cảm biến lên màn hình theo chu kỳ 2s một lần
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    // Kiểm tra lỗi đọc dữ liệu
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Error: Can't read data from DHT sensor!");
        return;
    }
    tempHumi();
    lv_chart_set_next_value(ui_TempHumiChart, ui_TempHumiChart_series_1, temperature);//Tên biểu đồ/giá trị cột trái/gía trị cột phải/Giá trị cảm biến
    lv_chart_set_next_value(ui_TempHumiChart, ui_TempHumiChart_series_2, humidity);
    lv_chart_refresh(ui_TempHumiChart); /*Làm mới biểu đồ*/


}

/**************************DTH-END************************/

void setup()
{
   Serial.begin( 115200 ); 
    String LVGL_Arduino = "LVGL version ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );

    lv_init();
    dht.begin();

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
#endif

    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); /* Start second SPI bus for touchscreen */
    ts.begin(mySpi); /* Touchscreen init */ 
    ts.setRotation(3); /* Landscape orientation *///Xoay tấm cảm ứng theo chiều ngang

    tft.begin();          /* TFT init */
    tft.setRotation(0); /* Landscape orientation *///Xoay màn hình theo chiều ngang

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * screenHeight / 10 );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_t * my_indev = lv_indev_drv_register( &indev_drv );

  ui_init();
  lv_timer_t* timer = lv_timer_create(my_timer,2000,NULL); //Đọc và gửi dữ liệu mỗi 2s
  Serial.println( "Setup done" );
}


void loop()
{ 

  lv_timer_handler();
  delay(5);
}
