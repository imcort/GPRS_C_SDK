#include "stdint.h"
#include "stdbool.h"
#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"
#include "api_hal_spi.h"
#include "api_hal_gpio.h"
#include "api_hal_pm.h"
#include "stdio.h"

#include "lcd.h"
#include "lvgl.h"

#define AppMain_TASK_STACK_SIZE    (1024 * 2)
#define AppMain_TASK_PRIORITY      0
HANDLE mainTaskHandle  = NULL;

lv_disp_drv_t disp_drv; 
bool lvgl_init_flag = false;
bool lvgl_flush_complete = true;

void EventDispatch(API_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case API_EVENT_ID_POWER_ON:
            break;
        case API_EVENT_ID_SYSTEM_READY:

            break;
        default:
            break;
    }
}

LCD_OP_t lcd = {
    .Open              = NULL,
    .Close             = NULL,
    .SetContrast       = NULL,
    .SetBrightness     = NULL,
    .SetStandbyMode    = NULL,
    .Sleep             = NULL,
    .PartialOn         = NULL,
    .PartialOff        = NULL,
    .WakeUp            = NULL,
    .GetScreenInfo     = NULL,
    .SetPixel16        = NULL,
    .FillRect16        = NULL,
    .Blit16            = NULL,
    .Busy              = NULL,
    .SetDirRotation    = NULL,
    .SetDirDefault     = NULL,
    .GetStringId       = NULL,
    .GetLcdId          = NULL,
};


void OnBlit(void* param)
{
    Trace(3,"blit ok");

    lvgl_flush_complete = true;
    
    //lv_disp_flush_ready(&disp_drv);         /* Indicate you are ready with the flushing*/
}

void Init_Interface()
{
    LCD_Screen_Info_t info;

    //register functions by lcd driver
    LCD_ili9341_Register(&lcd,OnBlit);
    lcd.Open();
    lcd.GetScreenInfo(&info);

    LCD_ROI_t roi = {
        .x = 0,
        .y = 0,
        .width = info.width,
        .height = info.height
    };
    lcd.FillRect16(&roi,0x0000);
}

void lvgl_task_tick()
{
    lv_tick_inc(1);
    OS_StartCallbackTimer(mainTaskHandle,1,lvgl_task_tick,NULL);
}

void my_disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p)
{
    LCD_FBW_t window;
    window.fb.buffer = (uint16_t *)color_p;
    // window.fb.buffer = (uint16_t*)color_p;
    window.fb.width = area->x2 - area->x1 + 1;
    window.fb.height = area->y2 - area->y1 + 1;
    window.fb.colorFormat = LCD_COLOR_FORMAT_RGB_565;
    window.roi.x = 0;
    window.roi.y = 0;
    window.roi.width = window.fb.width;
    window.roi.height = window.fb.height;
    lvgl_flush_complete = false;
    lcd.Blit16(&window, area->x1, area->y1);
    while(!lvgl_flush_complete) ;
    lv_disp_flush_ready(disp);
}

void Init_LVGL()
{
    lv_init();

    static lv_disp_buf_t disp_buf;                        /*Descriptor of a display driver*/
    static lv_color_t buf[LV_HOR_RES_MAX * 5];
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 5);                    /*Basic initialization*/

    
    lv_disp_drv_init(&disp_drv);          /*Basic initialization*/
    disp_drv.flush_cb = my_disp_flush;    /*Set your driver function*/
    disp_drv.buffer = &disp_buf;          /*Assign the buffer to the display*/
    lv_disp_drv_register(&disp_drv);      /*Finally register the driver*/

}

void LVGL_Task(void* param)
{
    Init_Interface();
    Init_LVGL();
    lvgl_init_flag = true;
    Trace(3,"init complete");
    // //show hello world and height 16 bits

    lv_obj_t * arc = lv_arc_create(lv_scr_act(), NULL);
    lv_arc_set_end_angle(arc, 200);
    lv_obj_set_size(arc, 150, 150);
    lv_obj_align(arc, NULL, LV_ALIGN_CENTER, 0, 0);

    /*************************************
     * Run the task handler of LittlevGL
     *************************************/
    while(1) {
        lv_task_handler();
        OS_Sleep(1);
    }
}

void Display_Task(void* param)
{
    while(!lvgl_init_flag)
        OS_Sleep(200);
    
    Trace(3,"start display");
    
    while(1)
    {
        OS_Sleep(1000);
    }
}

void AppMainTask(void *pData)
{
    API_Event_t* event=NULL;
    OS_Sleep(3000);
            
    OS_CreateTask(LVGL_Task ,
        NULL, NULL, AppMain_TASK_STACK_SIZE, AppMain_TASK_PRIORITY+1, 0, 0, "lvgl Task");
    OS_CreateTask(Display_Task ,
        NULL, NULL, AppMain_TASK_STACK_SIZE, AppMain_TASK_PRIORITY+2, 0, 0, "display Task");
    while(1)
    {
        if(OS_WaitEvent(mainTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}
void lvgl2_Main(void)
{
    mainTaskHandle = OS_CreateTask(AppMainTask ,
        NULL, NULL, AppMain_TASK_STACK_SIZE, AppMain_TASK_PRIORITY, 0, 0, "init Task");
    OS_SetUserMainHandle(&mainTaskHandle);
}
