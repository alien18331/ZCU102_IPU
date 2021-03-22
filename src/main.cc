/*  For description look into the help() function. */

#include "opencv2/core/core.hpp"
#include "opencv2/videoio/videoio.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>

#include <time.h>
#include <ctime>

#include "modbus/mb_tcp_server.h"
#include "modbus/mb_tcp_adu.h"
#include "modbus/mb_log.h"


//Set Mode
#define NORMAL_FLOW
//#define grayscale_compute
//#define time_compute
//#define repeat_max 100
#define delay_update
#define delay_ms 500

//Modbus IP Setting
#define HOST_ADDR  "192.168.2.100" //FPGA IP
#define HOST_PORT  502       /* using the standard port 502 requires root privileges */
#define AUTH_ADDR  "192.168.2.10"  /* authorised client address */

//Holding Registers Setting
#define HOLD_REG_ADDR   0x01
#define HOLD_REG_QUANT  1
uint16_t hold_reg[1] = {0xFFFF};

using namespace std;
using namespace cv;

double t1,t2;

static int handle(mb_tcp_server_t *server, mb_tcp_adu_t *req, mb_tcp_adu_t *resp)
{
    int ret = 0;

    switch (req->pdu.func_code)
    {
    case MB_PDU_RD_HOLD_REGS:
        if (req->pdu.rd_hold_regs_req.start_addr != HOLD_REG_ADDR)
        {
            return -MB_PDU_EXCEPT_ILLEGAL_ADDR;
        }
        if (req->pdu.rd_hold_regs_req.quant_regs != HOLD_REG_QUANT)
        {
            return -MB_PDU_EXCEPT_ILLEGAL_VAL;
        }
        mb_tcp_adu_set_header(resp, req->trans_id, req->proto_id, MB_TCP_SERVER_UNIT_ID);
        ret = mb_pdu_set_rd_hold_regs_resp(&resp->pdu, HOLD_REG_QUANT * 2, (const uint16_t*) &hold_reg);
        if (ret < 0)
        {
            return -MB_PDU_EXCEPT_SERVER_DEV_FAIL;
        }
        return ret;
    case MB_PDU_DIAG:
    	    switch (req->pdu.diag_req.sub_func)
    	    {
    	    	case 0x00: //return query data
    	    		mb_tcp_adu_set_header(resp, req->trans_id, req->proto_id, MB_TCP_SERVER_UNIT_ID);
    	    		ret = mb_pdu_set_diag_resp(&resp->pdu, req->pdu.diag_req.sub_func, req->pdu.diag_req.data, req->pdu.diag_req.num_data);
    	    		if (ret < 0)
        		{
            			return -MB_PDU_EXCEPT_SERVER_DEV_FAIL;
        		}
        		return ret;

        	default:
        		return -MB_PDU_EXCEPT_ILLEGAL_FUNC;
            }

    default:
        return -MB_PDU_EXCEPT_ILLEGAL_FUNC;
    }
    return 0;  /* should never reach here */
}

int modbus_server()
{
    mb_tcp_server_t server = {0};
    int ret = 0;

    mb_log_set_level(MB_LOG_DEBUG);
    ret = mb_tcp_server_create(&server, HOST_ADDR, HOST_PORT, handle);
    if (ret < 0)
    {
        mb_log_error("failed to create server: %s", strerror(-ret));
        return EXIT_FAILURE;
    }
    ret = mb_tcp_server_authorise_addr(&server, AUTH_ADDR);
    if (ret < 0)
    {
        mb_log_error("failed to authorise client address: %s", strerror(-ret));
        mb_tcp_server_destroy(&server);
        return EXIT_FAILURE;
    }
    ret = mb_tcp_server_run(&server);
    if (ret < 0)
    {
        mb_log_error("failed to run server: %s", strerror(-ret));
        mb_tcp_server_destroy(&server);
        return EXIT_FAILURE;
    }
    mb_tcp_server_destroy(&server);
    return EXIT_SUCCESS;
}

int runIPU(int num,char** name)
{

	uint16_t repeat_time=0;
	double capture_total=0,bram_total=0,ebr_total=0,save_image_total=0;
	VideoCapture capture(0);
#ifdef time_compute
	while(repeat_time<repeat_max)
#else
	while(1)
#endif
	{
		Mat srcImg, grayImg;
		int fd;
		unsigned int bram_size = 0x100000;
		off_t bram_pbase = 0xa0000000; // physical base address
		uint32_t *bram32_vptr;
		uint8_t buf[480], EBR_range;
		double start,capture_end, bram_save_end,EBR_end,image_save_end;

		char filename[100];
		snprintf(filename, sizeof(filename), "%s.bmp", name[1]);

		start = (double) getTickCount(); // 1clock = 1ns
		if ( !capture.isOpened( ) )
		{
		        cout << "fail to open camera!" << endl;
		        return 1;
		}
		capture>>srcImg;

	#ifndef NORMAL_FLOW
		if(num==2)
			imwrite(filename, srcImg);
		else
			imwrite("test.bmp", srcImg);
	#endif
		capture_end = (double) getTickCount();

	#ifndef NORMAL_FLOW
		FILE *fp;
		if(num==2)
			fp=fopen(filename,"rb"); // 640 x 480
		else
			fp=fopen("img_sample.bmp","rb"); // 640 x 480
		fread(buf,sizeof(char),54,fp);
	#endif
		// Map the BRAM physical address into user space getting a virtual address for it
		if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) != -1)
		{

			bram32_vptr = (uint32_t *)mmap(NULL, bram_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, bram_pbase);
			bram32_vptr[100002] = 0x00;
	#ifdef NORMAL_FLOW
			uint32_t gray_temp,gray;
			uint8_t* p = srcImg.data;

			for(int i=479;i>=0;i--)
			{
				uint8_t* p_temp;
				p_temp	= p + i*1920 ;
				for(int j=0;j<160;j++)
				{
					uint32_t index;
					index = 12*j;
	#ifdef grayscale_compute
					gray_temp = (p_temp[0+index] * 30 + p_temp[1+index] * 59 + p_temp[2+index] * 11 + 50) / 100 ;
					gray = gray_temp<<24;

					gray_temp = (p_temp[3+index] * 30 + p_temp[4+index] * 59 + p_temp[5+index] * 11 + 50) / 100 ;
					gray += gray_temp<<16;

					gray_temp = (p_temp[6+index] * 30 + p_temp[7+index] * 59 + p_temp[8+index] * 11 + 50) / 100 ;
					gray += gray_temp<<8;

					gray_temp = (p_temp[9+index] * 30 + p_temp[10+index] * 59 + p_temp[11+index] * 11 + 50) / 100 ;
					gray += gray_temp;
	#else
					gray = (p_temp[0+index]<<24) + (p_temp[3+index]<<16) + (p_temp[6+index]<<8) + p_temp[9+index] ;
	#endif
					bram32_vptr[j + 76640 - i*160 ] = gray ;
				}
			}

	#endif
	#ifndef NORMAL_FLOW
			for (int j=0;j<640*3;j++)
			{
				fread(buf,sizeof(char),160*3,fp);
				for(int i=0;i<40;i++)
				{
					uint32_t gray_temp,gray,red,green,blue,index;
					index = i*12 ;
	#ifdef grayscale_compute
					red   = buf[index];
					green = buf[index+1];
					blue  = buf[index+2];
					gray_temp = (30 * red + green * 59 + blue * 11 + 50)/100 ;
					gray = gray_temp<<24 ;

					red   = buf[index+3];
					green = buf[index+4];
					blue  = buf[index+5];
					gray_temp = (30 * red + green * 59 + blue * 11 + 50)/100 ;
					gray += gray_temp<<16 ;

					red   = buf[index+6];
					green = buf[index+7];
					blue  = buf[index+8];
					gray_temp = (30 * red + green * 59 + blue * 11 + 50)/100 ;
					gray += gray_temp<<8 ;

					red   = buf[index+9];
					green = buf[index+10];
					blue  = buf[index+11];
					gray_temp = (30 * red + green * 59 + blue * 11 + 50)/100 ;
					gray += gray_temp ;
	#else
					gray = (buf[index]<<24) + (buf[index+3]<<16) + (buf[index+6]<<8) + (buf[index+9]) ;
	#endif
					bram32_vptr[40*j+i] = gray;
				}
			}
	#endif

			bram_save_end = (double) getTickCount();
			bram32_vptr[100003] = 0x01;
			while(bram32_vptr[100002]!=1);

			EBR_range = bram32_vptr[100000];
			EBR_end = (double) getTickCount();

			munmap(bram32_vptr, bram_size);

	#ifdef NORMAL_FLOW
			if(num==2)
				imwrite(filename, srcImg);
			else
				imwrite("test.bmp", srcImg);
			image_save_end = (double) getTickCount();
	#endif
/*
    #ifdef time_compute
			printf("EBR_range=%d\r\n",EBR_range);
			printf("capture_end - start = %f ms \r\n",(capture_end-start)/1000000);
			printf("bram_save_end - capture_end = %f us \r\n",(bram_save_end-capture_end)/1000);
			printf("EBR_end - bram_save_end = %f us \r\n",(EBR_end-bram_save_end)/1000);
	#ifdef NORMAL_FLOW
			printf("image_save_end - EBR_end = %f us \r\n",(image_save_end-EBR_end)/1000);
	#endif
	#endif
*/
			hold_reg[0] = EBR_range ;
			close(fd);
		}
		else
		{
			cout<<"fail to open BRAM!"<<endl;
			return 1;
		}
	#ifdef time_compute
		capture_total +=(capture_end-start);
		bram_total += (bram_save_end-capture_end);
		ebr_total += (EBR_end-bram_save_end);
	#ifdef NORMAL_FLOW
		save_image_total += image_save_end-EBR_end ;
	#endif
		repeat_time++;
		printf("%d times\r\n",repeat_time);
	#endif

	#ifdef delay_update
		double w1,w2;
		w1 = (double)getTickCount();
		w2 = (double)getTickCount();
		while(w2-w1 < delay_ms*1000000)
			w2 = (double)getTickCount();
	#endif
	}

#ifdef time_compute
	t2 = (double)getTickCount();
	printf("run %d times = %f s \r\n",repeat_max,(t2-t1)/1000000000);
	printf("capture_total %f s \r\n",capture_total/1000000000);
	printf("bram_total  = %f s \r\n",bram_total/1000000000);
	printf("ebr_total = %f ms \r\n",ebr_total/1000000);
#ifdef NORMAL_FLOW
	printf("save_image_total = %f s \r\n",save_image_total/1000000000);
#endif

#endif
	capture.release();
	return 0 ;
}



int main(int num,char** name)
{

	t1 = (double) getTickCount();

	thread mT1(runIPU,num,name);
	thread mT2(modbus_server);
	mT1.join();
	mT2.join();
    return 0;
}
