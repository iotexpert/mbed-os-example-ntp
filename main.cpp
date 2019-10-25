#include "mbed.h"
#include "GUI.h"
#include "mbed_events.h"
#include "ntp-client/NTPClient.h"


Thread netTimeThreadHandle;
WiFiInterface *wifi;
EventQueue *displayQueue;
Semaphore WiFiSemaphore;
/******************************************************************************************
 *
 * Display Functions
 *
 ********************************************************************************************/
#define DISP_LEFTMARGIN 10
#define DISP_TOPMARGIN 4
#define DISP_LINESPACE 2
// updateDisplayWiFiStatus
// Used to display the wifi status
void updateDisplayWiFiStatus(char *status)
{
  GUI_SetFont(GUI_FONT_16_1);
  GUI_DispStringAt(status,DISP_LEFTMARGIN, DISP_TOPMARGIN); 
  free(status);  
}
// updateDisplayWiFiConnectAttempts
// This function displays the number of attempted connections
void updateDisplayWiFiConnectAttempts(int count)
{
  char buffer[128];
  snprintf(buffer,sizeof(buffer),"WiFi Connect Attempts = %d",count); 
  GUI_SetFont(GUI_FONT_16_1);
  GUI_DispStringAt(buffer,DISP_LEFTMARGIN, DISP_TOPMARGIN + (GUI_GetFontSizeY()+DISP_LINESPACE) ); 
}
// updateDisplayNTPCount
// updates the display with the number of time the NTP Server has been called
void updateDisplayNTPCount(void)
{
  static int count=0;
  char buffer[128];
  count = count + 1;
  snprintf(buffer,sizeof(buffer),"NTP Updates = %d\n",count);
  GUI_SetFont(GUI_FONT_16_1);
  GUI_DispStringHCenterAt(buffer,LCD_GetXSize()/2,LCD_GetYSize() - GUI_GetFontSizeY()); // near the bottom
}
// updateDisplayTime
// This function updates the time on the screen
void updateDisplayTime()
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [128];
  time (&rawtime);
  rawtime = rawtime - (4*60*60); // UTC - 4hours ... serious hack which only works in summer
  timeinfo = localtime (&rawtime);
  strftime (buffer,sizeof(buffer),"%r",timeinfo);
  GUI_SetFont(GUI_FONT_32B_1);
  GUI_DispStringHCenterAt(buffer,LCD_GetXSize()/2,LCD_GetYSize()/2 - GUI_GetFontSizeY()/2);
}
/******************************************************************************************
 * NTPTimeThread
 * This thread calls the NTP Timeserver to get the UTC time
 * It then updates the time in the RTC
 * And it updates the display by adding an event to the display queue
 ********************************************************************************************/
void NTPTimeThread()
{
  NTPClient ntpclient(wifi);
  while(1)
    {
      if(wifi->get_connection_status() == NSAPI_STATUS_GLOBAL_UP)
	{
	  time_t timestamp = ntpclient.get_timestamp();
	  if (timestamp < 0) {
	    // probably need to do something different here
	  } 
	  else 
	    {
	      set_time(timestamp);
	      displayQueue->call(updateDisplayNTPCount);
	    }
	}
      wait(60.0*5); // Goto the NTP server every 5 minutes
    }
}
/******************************************************************************************
 *
 * Main & WiFi Thread
 *
 ********************************************************************************************/
// wifiStatusCallback
// Changes the display when the wifi status is changed
void wifiStatusCallback(nsapi_event_t status, intptr_t param)
{
  const int buffSize=40;
  char *statusText;
  statusText = (char *)malloc(buffSize);
  switch(param) {
  case NSAPI_STATUS_LOCAL_UP:
    snprintf(statusText,buffSize,"WiFi IP = %s",wifi->get_ip_address());
    break;
  case NSAPI_STATUS_GLOBAL_UP:
    snprintf(statusText,buffSize,"WiFi IP = %s",wifi->get_ip_address());
    break;
  case NSAPI_STATUS_DISCONNECTED:
    WiFiSemaphore.release();
    snprintf(statusText,buffSize,"WiFi Disconnected");
    break;
  case NSAPI_STATUS_CONNECTING:
    snprintf(statusText,buffSize,"WiFi Connecting");
    break;
  default:
    snprintf(statusText,buffSize,"Not Supported");
    break;
  }
  displayQueue->call(updateDisplayWiFiStatus,statusText);
}
int main()
{
  int wifiConnectionAttempts;
  int ret;
  GUI_Init();
  displayQueue = mbed_event_queue();
  displayQueue->call_every(1000, &updateDisplayTime);
  wifi = WiFiInterface::get_default_instance();
  wifi->attach(&wifiStatusCallback);
  while(1)
    {
      wifiConnectionAttempts = 1;
      do {
	ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
	displayQueue->call(updateDisplayWiFiConnectAttempts,wifiConnectionAttempts);
	if (ret != 0) {
	  wifiConnectionAttempts += 1;
	  wait(2.0); // If for some reason it doesnt work wait 2s and try again
	}
      } while(ret !=0);
      // If the NTPThread is not running... then start it up
      if(netTimeThreadHandle.get_state() == Thread::Deleted)
	netTimeThreadHandle.start(NTPTimeThread);
      WiFiSemaphore.acquire();
    }
}
