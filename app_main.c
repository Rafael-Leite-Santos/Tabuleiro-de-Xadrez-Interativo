#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "Xadrez";

#define BLINK_GPIO 12
#define NUM_LEDS 64  // Número de LEDs na tira

static led_strip_handle_t led_strip;

char Jogada[3];
int linx, colx; // coordenadas que são atualizadas todas as vezes chamadas com contr==0
int liny, coly; // coordendas que são atualizadas todas as vezes que chama a funçao casa
int contr = 0; // controle
int cor = 1; // Contador de jogadas
char conf;
char rei_P = 1; // Quando for a primeira jogada do rei preto
char rei_B = 1; // Quando for a primeira jogada do rei branco
char roque_d = 0;
char roque_e = 0;

char Tabuleiro[8][8] = {{'t','c','b','d','r','b','c','t'},  //8
                        {'p','p','p','p','p','p','p','p'},  //7
                        { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },  //6
                        { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },  //5
                        { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },  //4
                        { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },  //3
                        {'P','P','P','P','P','P','P','P'},  //2
                        {'T','C','B','D','R','B','C','T'}}; //1
                      //  A , B , C , D , E , F , G , H 
int led [8][8] = {{ 56, 57, 58, 59, 60, 61, 62, 63},  //8
                  { 55, 54, 53, 52, 51, 50, 49, 48},  //7
                  { 40, 41, 42, 43, 44, 45, 46, 47},  //6
                  { 39, 38, 37, 36, 35, 34, 33, 32},  //5 
                  { 24, 25, 26, 27, 28, 29, 30, 31},  //4
                  { 23, 22, 21, 20, 19, 18, 17, 16},  //3
                  { 8 , 9 , 10, 11, 12, 13, 14, 15},  //2
                  { 7 , 6 , 5 , 4 , 3 , 2 , 1 , 0 }}; //1
                 // A , B , C , D , E , F , G , H 

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configure LED");
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = NUM_LEDS, // Define o número de LEDs
    };
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));

    led_strip_clear(led_strip);
}

void Movimento(int lin, int col)
{
    int i;
    printf("Funçao movimento\n");
    if (contr == 0)
    {
        led_strip_set_pixel(led_strip,(led[lin][col]),255,255,255);
        switch(Tabuleiro[lin][col])
        {
            case 'p': // Peão preto OK
                if(lin == 1)
                {
                    for(i=1;i<3;i++)
                    {
                        if(Tabuleiro[lin+i][col]==0)
                        {
                            if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                            {
                                printf("LED: %d azul\n", led[lin+i][col]);
                                led_strip_set_pixel(led_strip,led[lin+i][col],0,0,255);
                            }
                        }
                    }
                } else if(Tabuleiro[lin+1][col]==0)
                {
                    if(led[lin+1][col]>-1&&led[lin+1][col]<64)
                    {
                        printf("LED: %d azul\n", led[lin+1][col]);
                        led_strip_set_pixel(led_strip,led[lin+1][col],0,0,255);
                    }
                }
                for (i=-1;i<2;)
                {
                    if((Tabuleiro[lin+1][col+i]>65)&&(Tabuleiro[lin+1][col+i]<91))
                    {
                        if(led[lin+1][col+i]>-1&&led[lin+1][col+i]<64)
                        {
                            printf("LED: %d azul\n", led[lin+1][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+1][col+i],0,0,255);
                        }
                    }
                    i=i+2;
                }
            break;
            case 't': // Torre preta OK
                for(i=1;i<(col);i++)
                {
                    if (i==8||(col-i<0)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin][col-i]==0) //esquerda
                    {
                        if((led[lin][col-i]>-1)&&(led[lin][col-i]<64))
                        {
                            printf("Led a acender: %d Azul\n", led[lin][col-i]);
                            led_strip_set_pixel(led_strip,led[lin][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin][col-i]>65)&&(Tabuleiro[lin][col-i]<91))
                    {
                        if(led[lin][col-i]>-1&&led[lin][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin][col-i]);
                            led_strip_set_pixel(led_strip,led[lin][col-i],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin][col-i]>96)&&(Tabuleiro[lin][col-i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-lin);i++)
                {
                    if (i==8||(lin+i<0)||(lin+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col]==0) //acima
                    {
                        if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col]);
                            led_strip_set_pixel(led_strip,led[lin+i][col],0,0,255);
                        }
                    } else if ((Tabuleiro[lin+i][col]>65)&&(Tabuleiro[lin+i][col]<91))
                    {
                        if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col]);
                            led_strip_set_pixel(led_strip,led[lin+i][col],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin+i][col]>96)&&(Tabuleiro[lin+i][col]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-col);i++)
                {
                    if (i==8||(col+i<0)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin][col+i]==0) //direita
                    {
                        if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin][col+i]);
                            led_strip_set_pixel(led_strip,led[lin][col+i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin][col+i]>65)&&(Tabuleiro[lin][col+i]<91))
                    {
                        if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin][col+i]);
                            led_strip_set_pixel(led_strip,led[lin][col+i],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin][col+i]>96)&&(Tabuleiro[lin][col+i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-lin);i++)
                {
                    if (i==8||(lin-i<0)||(lin-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col]==0) //abaixo
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col]);
                            led_strip_set_pixel(led_strip,led[lin-i][col],0,0,255);
                        }
                    } else if ((Tabuleiro[lin-i][col]>65)&&(Tabuleiro[lin-i][col]<91))
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col]);
                            led_strip_set_pixel(led_strip,led[lin-i][col],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin-i][col]>96)&&(Tabuleiro[lin-i][col]<123))
                    {
                        break;
                    }
                }
            break;
            case 'c': // Cavalo preto OK
                if (Tabuleiro[lin+2][col+1]==0)
                {
                    if ((lin+2<8)&&(col+1<8)&&(lin+2>-1)&&(col+1>-1))
                    {
                        if(led[lin+2][col+1]>-1&&led[lin+2][col+1]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+2][col+1]);
                            led_strip_set_pixel(led_strip,led[lin+2][col+1],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin+2][col+1]>65)&&(Tabuleiro[lin+2][col+1]<91))
                {
                    if ((lin+2<8)&&(col+1<8)&&(lin+2>-1)&&(col+1>-1))
                    {
                        if(led[lin+2][col+1]>-1&&led[lin+2][col+1]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+2][col+1]);
                            led_strip_set_pixel(led_strip,led[lin+2][col+1],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin+2][col-1]==0)
                {
                    if((lin+2<8)&&(col-1<8)&&(lin+2>-1)&&(col-1>-1))
                    {
                        if(led[lin+2][col-1]>-1&&led[lin+2][col-1]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+2][col-1]);
                            led_strip_set_pixel(led_strip,led[lin+2][col-1],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin+2][col-1]>65)&&(Tabuleiro[lin+2][col-1]<91))
                {
                    if((lin+2<8)&&(col-1<8)&&(lin+2>-1)&&(col-1>-1))
                    {
                        if(led[lin+2][col-1]>-1&&led[lin+2][col-1]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+2][col-1]);
                            led_strip_set_pixel(led_strip,led[lin+2][col-1],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin-2][col+1]==0)
                {
                    if ((lin-2<8)&&(col+1<8)&&(lin-2>-1)&&(col+1>-1))
                    {
                        if(led[lin-2][col+1]>-1&&led[lin-2][col+1]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-2][col+1]);
                            led_strip_set_pixel(led_strip,led[lin-2][col+1],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin-2][col+1]>65)&&(Tabuleiro[lin-2][col+1]<91))
                {
                    if ((lin-2<8)&&(col+1<8)&&(lin-2>-1)&&(col+1>-1))
                    {
                        if(led[lin-2][col+1]>-1&&led[lin-2][col+1]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-2][col+1]);
                            led_strip_set_pixel(led_strip,led[lin-2][col+1],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin-2][col-1]==0)
                {
                    if((lin-2<8)&&(col-1<8)&&(lin-2>-1)&&(col-1>-1))
                    {
                        if(led[lin-2][col-1]>-1&&led[lin-2][col-1]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-2][col-1]);
                            led_strip_set_pixel(led_strip,led[lin-2][col-1],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin-2][col-1]>65)&&(Tabuleiro[lin-2][col-1]<91))
                {
                    if((lin-2<8)&&(col-1<8)&&(lin-2>-1)&&(col-1>-1))
                    {
                        if(led[lin-2][col-1]>-1&&led[lin-2][col-1]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-2][col-1]);
                            led_strip_set_pixel(led_strip,led[lin-2][col-1],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin+1][col+2]==0)
                {
                    if((lin+1<8)&&(col+2<8)&&(lin+1>-1)&&(col+2>-1))
                    {
                        if(led[lin+1][col+2]>-1&&led[lin+1][col+2]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+1][col+2]);
                            led_strip_set_pixel(led_strip,led[lin+1][col+2],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin+1][col+2]>65)&&(Tabuleiro[lin+1][col+2]<91))
                {
                    if((lin+1<8)&&(col+2<8)&&(lin+1>-1)&&(col+2>-1))
                    {
                        if(led[lin+1][col+2]>-1&&led[lin+1][col+2]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+1][col+2]);
                            led_strip_set_pixel(led_strip,led[lin+1][col+2],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin-1][col+2]==0)
                {
                    if((lin-1<8)&&(col+2<8)&&(lin-1>-1)&&(col+2>-1))
                    {
                        if(led[lin-1][col+2]>-1&&led[lin-1][col+2]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-1][col+2]);
                            led_strip_set_pixel(led_strip,led[lin-1][col+2],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin-1][col+2]>65)&&(Tabuleiro[lin-1][col+2]<91))
                {
                    if((lin-1<8)&&(col+2<8)&&(lin-1>-1)&&(col+2>-1))
                    {
                        if(led[lin-1][col+2]>-1&&led[lin-1][col+2]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-1][col+2]);
                            led_strip_set_pixel(led_strip,led[lin-1][col+2],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin+1][col-2]==0)
                {
                    if((lin+1<8)&&(col-2<8)&&(lin+1>-1)&&(col-2>-1))
                    {
                        if(led[lin+1][col-2]>-1&&led[lin+1][col-2]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+1][col-2]);
                            led_strip_set_pixel(led_strip,led[lin+1][col-2],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin+1][col-2]>65)&&(Tabuleiro[lin+1][col-2]<91))
                {
                    if((lin+1<8)&&(col-2<8)&&(lin+1>-1)&&(col-2>-1))
                    {
                        if(led[lin+1][col-2]>-1&&led[lin+1][col-2]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+1][col-2]);
                            led_strip_set_pixel(led_strip,led[lin+1][col-2],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin-1][col-2]==0)
                {
                    if((lin-1<8)&&(col-2<8)&&(lin-1>0)&&(col-2>-1))
                    {
                        if(led[lin-1][col-2]>-1&&led[lin-1][col-2]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-1][col-2]);
                            led_strip_set_pixel(led_strip,led[lin-1][col-2],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin-1][col-2]>65)&&(Tabuleiro[lin-1][col-2]<91))
                {
                    if((lin-1<8)&&(col-2<8)&&(lin-1>-1)&&(col-2>-1))
                    {
                        if(led[lin-1][col-2]>-1&&led[lin-1][col-2]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-1][col-2]);
                            led_strip_set_pixel(led_strip,led[lin-1][col-2],255,0,0);
                        }
                    }
                }
            break;
            case 'b': //Bispo preto OK
                for (i=1;i<8;i++)
                {
                    if ((lin-i<0)||(col-i<0)||(lin-i>7)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col-i]==0) //diagonal esquerda inferior
                    {
                        if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin-i][col-i]>65)&&(Tabuleiro[lin-i][col-i]<91))
                    {
                        if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col-i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin-i][col-i]>96)&&(Tabuleiro[lin-i][col-i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin+i<0)||(col-i<0)||(lin+i>7)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col-i]==0) //diagnoal esquerda superior
                    {
                        if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin+i][col-i]>65)&&(Tabuleiro[lin+i][col-i]<91))
                    {
                        if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col-i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin+i][col-i]>96)&&(Tabuleiro[lin+i][col-i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin+i<0)||(col+i<0)||(lin+i>7)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col+i]==0) //diagonal direita superior
                    {
                        if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col+i],0,0,255);
                        }
                    }else if ((Tabuleiro[lin+i][col+i]>65)&&(Tabuleiro[lin+i][col+i]<91))
                    {
                        if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col+i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin+i][col+i]>96)&&(Tabuleiro[lin+i][col+i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin-i<0)||(col+i<0)||(lin-i>7)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col+i]==0) //diagonal direita inferior
                    {
                        if(led[lin-i][col+i]>-1&&led[lin-i][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col+i],0,0,255);
                        }
                    }else if ((Tabuleiro[lin-i][col+i]>65)&&(Tabuleiro[lin-i][col+i]<91))
                    {
                        if(led[lin-i][col+i]>-1&&led[lin-i][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col+i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin-i][col+i]>96)&&(Tabuleiro[lin-i][col+i]<123))
                    {
                        break;
                    }
                }
            break;
            case 'd': // Dama preta
                for (i=1;i<8;i++)
                {
                    if ((lin-i<0)||(col-i<0)||(lin-i>7)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col-i]==0) //diagonal esquerda inferior
                    {
                        if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin-i][col-i]>65)&&(Tabuleiro[lin-i][col-i]<91))
                    {
                        if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col-i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin-i][col-i]>96)&&(Tabuleiro[lin-i][col-i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin+i<0)||(col-i<0)||(lin+i>7)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col-i]==0) //diagnoal esquerda superior
                    {
                        if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin+i][col-i]>65)&&(Tabuleiro[lin+i][col-i]<91))
                    {
                        if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col-i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin+i][col-i]>96)&&(Tabuleiro[lin+i][col-i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin+i<0)||(col+i<0)||(lin+i>7)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col+i]==0) //diagonal direita superior
                    {
                        if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col+i],0,0,255);
                        }
                    }else if ((Tabuleiro[lin+i][col+i]>65)&&(Tabuleiro[lin+i][col+i]<91))
                    {
                        if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col+i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin+i][col+i]>96)&&(Tabuleiro[lin+i][col+i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin-i<0)||(col+i<0)||(lin-i>7)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col+i]==0) //diagonal direita inferior
                    {
                        if(led[lin-i][col+i]>-1&&led[lin-i][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col+i],0,0,255);
                        }
                    }else if ((Tabuleiro[lin-i][col+i]>65)&&(Tabuleiro[lin-i][col+i]<91))
                    {
                        if(led[lin-i][col+i]>-1&&led[lin-i][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col+i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin-i][col+i]>96)&&(Tabuleiro[lin-i][col+i]<123))
                    {
                        break;
                    }
                }
                for(i=1;i<(col);i++)
                {
                    if (i==8||(col-i<0))
                    {
                        break;
                    }
                    if (Tabuleiro[lin][col-i]==0) //esquerda
                    {
                        if((led[lin][col-i]>-1)&&(led[lin][col-i]<64))
                        {
                            printf("Led a acender: %d Azul\n", led[lin][col-i]);
                            led_strip_set_pixel(led_strip,led[lin][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin][col-i]>65)&&(Tabuleiro[lin][col-i]<91))
                    {
                        if(led[lin][col-i]>-1&&led[lin][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin][col-i]);
                            led_strip_set_pixel(led_strip,led[lin][col-i],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin][col-i]>96)&&(Tabuleiro[lin][col-i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-lin);i++)
                {
                    if (i==8||(lin+i<0))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col]==0) //acima
                    {
                        if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col]);
                            led_strip_set_pixel(led_strip,led[lin+i][col],0,0,255);
                        }
                    } else if ((Tabuleiro[lin+i][col]>65)&&(Tabuleiro[lin+i][col]<91))
                    {
                        if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col]);
                            led_strip_set_pixel(led_strip,led[lin+i][col],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin+i][col]>96)&&(Tabuleiro[lin+i][col]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-col);i++)
                {
                    if (i==8||(col+i<0))
                    {
                        break;
                    }
                    if (Tabuleiro[lin][col+i]==0) //direita
                    {
                        if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin][col+i]);
                            led_strip_set_pixel(led_strip,led[lin][col+i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin][col+i]>65)&&(Tabuleiro[lin][col+i]<91))
                    {
                        if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin][col+i]);
                            led_strip_set_pixel(led_strip,led[lin][col+i],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin][col+i]>96)&&(Tabuleiro[lin][col+i]<123))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-lin);i++)
                {
                    if (i==8||(lin-i<0))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col]==0) //abaixo
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col]);
                            led_strip_set_pixel(led_strip,led[lin-i][col],0,0,255);
                        }
                    } else if ((Tabuleiro[lin-i][col]>65)&&(Tabuleiro[lin-i][col]<91))
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col]);
                            led_strip_set_pixel(led_strip,led[lin-i][col],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin-i][col]>96)&&(Tabuleiro[lin-i][col]<123))
                    {
                        break;
                    }
                }
            break;
            case 'r': // Rei preto OK
                i=1;
                if (Tabuleiro[lin-i][col-i]==0) //diagonal esquerda inferior
                {
                    if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin-i][col-i]);
                        led_strip_set_pixel(led_strip,led[lin-i][col-i],0,0,255);
                    }
                } else if ((Tabuleiro[lin-i][col-i]>65)&&(Tabuleiro[lin-i][col-i]<91))
                {
                    if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin-i][col-i]);
                        led_strip_set_pixel(led_strip,led[lin-i][col-i],255,0,0);
                    }
                }
                if (Tabuleiro[lin][col-i]==0) //esquerda
                {
                    if(led[lin][col-i]>-1&&led[lin][col-i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin][col-i]);
                        led_strip_set_pixel(led_strip,led[lin][col-i],0,0,255);
                    }
                } else if ((Tabuleiro[lin][col-i]>65)&&(Tabuleiro[lin][col-i]<91))
                {
                    if(led[lin][col-i]>-1&&led[lin][col-i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin][col-i]);
                        led_strip_set_pixel(led_strip,led[lin][col-i],255,0,0);
                    }
                }
                if (Tabuleiro[lin+i][col-i]==0) //diagnoal esquerda superior
                {
                    if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin+i][col-i]);
                        led_strip_set_pixel(led_strip,led[lin+i][col-i],0,0,255);
                    }
                } else if ((Tabuleiro[lin+i][col-i]>65)&&(Tabuleiro[lin+i][col-i]<91))
                {
                    if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin+i][col-i]);
                        led_strip_set_pixel(led_strip,led[lin+i][col-i],255,0,0);
                    }
                }
                if (Tabuleiro[lin+i][col]==0) //acima
                {
                    if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin+i][col]);
                        led_strip_set_pixel(led_strip,led[lin+i][col],0,0,255);
                    }
                } else if ((Tabuleiro[lin+i][col]>65)&&(Tabuleiro[lin+i][col]<91))
                {
                    if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin+i][col]);
                        led_strip_set_pixel(led_strip,led[lin+i][col],255,0,0);
                    }
                }
                if (Tabuleiro[lin+i][col+i]==0) //diagonal direita superior
                {
                    if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin+i][col+i]);
                        led_strip_set_pixel(led_strip,led[lin+i][col+i],0,0,255);
                    }
                } else if ((Tabuleiro[lin+i][col+i]>65)&&(Tabuleiro[lin+i][col+i]<91))
                {
                    if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin+i][col+i]);
                        led_strip_set_pixel(led_strip,led[lin+i][col+i],255,0,0);
                    }
                }
                if (Tabuleiro[lin][col+i]==0) //direita
                {
                    if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin][col+i]);
                        led_strip_set_pixel(led_strip,led[lin][col+i],0,0,255);
                    }
                } else if ((Tabuleiro[lin][col+i]>65)&&(Tabuleiro[lin][col+i]<91))
                {
                    if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin][col+i]);
                        led_strip_set_pixel(led_strip,led[lin][col+i],255,0,0);
                    }
                }
                if (Tabuleiro[lin-1][col+i]==0) //diagonal direita inferior
                {
                    if(led[lin-1][col+i]>-1&&led[lin-1][col+i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin-1][col+i]);
                        led_strip_set_pixel(led_strip,led[lin-1][col+i],0,0,255);
                    }
                } else if ((Tabuleiro[lin-1][col+i]>65)&&(Tabuleiro[lin-1][col+i]<91))
                {
                    if(led[lin-1][col+i]>-1&&led[lin-1][col+i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin-1][col+i]);
                        led_strip_set_pixel(led_strip,led[lin-1][col+i],255,0,0);
                    }
                }
                if (Tabuleiro[lin-i][col]==0) //abaixo
                {
                    if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin-i][col]);
                        led_strip_set_pixel(led_strip,led[lin-i][col],0,0,255);
                    }
                } else if ((Tabuleiro[lin-i][col]>65)&&(Tabuleiro[lin-i][col]<91))
                {
                    if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin-i][col]);
                        led_strip_set_pixel(led_strip,led[lin-i][col],255,0,0);
                    }
                }
            break;
            case 'P': // Peão branco
                if(lin == 6)
                {
                    for(i=1;i<3;i++)
                    {
                        if(Tabuleiro[lin-i][col]==0)
                        {
                            if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                            {
                                printf("LED: %d azul\n", led[lin-i][col]);
                                led_strip_set_pixel(led_strip,led[lin-i][col],0,0,255);
                            }
                        }
                    }
                } else if(Tabuleiro[lin-1][col]==0)
                {
                    if(led[lin-1][col]>-1&&led[lin-1][col]<64)
                    {
                        printf("LED: %d azul\n", led[lin-1][col]);
                        led_strip_set_pixel(led_strip,led[lin-1][col],0,0,255);
                    }
                }
                for (i=-1;i<2;)
                {
                    if((Tabuleiro[lin-1][col+i]>96)&&(Tabuleiro[lin-1][col+i]<123))
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("LED: %d red\n", led[lin-1][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-1][col+i],255,0,0);
                        }
                    }
                    i=i+2;
                }
                break;
            case 'T': // Torre branca
                for(i=1;i<(col);i++)
                {
                    if (i==8||(col-i<0)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin][col-i]==0) //esquerda
                    {
                        if((led[lin][col-i]>-1)&&(led[lin][col-i]<64))
                        {
                            printf("Led a acender: %d Azul\n", led[lin][col-i]);
                            led_strip_set_pixel(led_strip,led[lin][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin][col-i]>96)&&(Tabuleiro[lin][col-i]<123))
                    {
                        if(led[lin][col-i]>-1&&led[lin][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin][col-i]);
                            led_strip_set_pixel(led_strip,led[lin][col-i],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin][col-i]>65)&&(Tabuleiro[lin][col-i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-lin);i++)
                {
                    if (i==8||(lin+i<0)||(lin+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col]==0) //acima
                    {
                        if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col]);
                            led_strip_set_pixel(led_strip,led[lin+i][col],0,0,255);
                        }
                    } else if ((Tabuleiro[lin+i][col]>96)&&(Tabuleiro[lin+i][col]<123))
                    {
                        if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col]);
                            led_strip_set_pixel(led_strip,led[lin+i][col],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin+i][col]>65)&&(Tabuleiro[lin+i][col]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-col);i++)
                {
                    if (i==8||(col+i<0)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin][col+i]==0) //direita
                    {
                        if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin][col+i]);
                            led_strip_set_pixel(led_strip,led[lin][col+i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin][col+i]>96)&&(Tabuleiro[lin][col+i]<123))
                    {
                        if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin][col+i]);
                            led_strip_set_pixel(led_strip,led[lin][col+i],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin][col+i]>65)&&(Tabuleiro[lin][col+i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-lin);i++)
                {
                    if (i==8||(lin-i<0)||(lin-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col]==0) //abaixo
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col]);
                            led_strip_set_pixel(led_strip,led[lin-i][col],0,0,255);
                        }
                    } else if ((Tabuleiro[lin-i][col]>96)&&(Tabuleiro[lin-i][col]<123))
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col]);
                            led_strip_set_pixel(led_strip,led[lin-i][col],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin-i][col]>65)&&(Tabuleiro[lin-i][col]<91))
                    {
                        break;
                    }
                }
            break;
            case 'C': // Cavalo branco
                if (Tabuleiro[lin+2][col+1]==0)
                {
                    if ((lin+2<8)&&(col+1<8)&&(lin+2>-1)&&(col+1>-1))
                    {
                        if(led[lin+2][col+1]>-1&&led[lin+2][col+1]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+2][col+1]);
                            led_strip_set_pixel(led_strip,led[lin+2][col+1],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin+2][col+1]>96)&&(Tabuleiro[lin+2][col+1]<123))
                {
                    if ((lin+2<8)&&(col+1<8)&&(lin+2>-1)&&(col+1>-1))
                    {
                        if(led[lin+2][col+1]>-1&&led[lin+2][col+1]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+2][col+1]);
                            led_strip_set_pixel(led_strip,led[lin+2][col+1],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin+2][col-1]==0)
                {
                    if((lin+2<8)&&(col-1<8)&&(lin+2>-1)&&(col-1>-1))
                    {
                        if(led[lin+2][col-1]>-1&&led[lin+2][col-1]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+2][col-1]);
                            led_strip_set_pixel(led_strip,led[lin+2][col-1],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin+2][col-1]>96)&&(Tabuleiro[lin+2][col-1]<123))
                {
                    if((lin+2<8)&&(col-1<8)&&(lin+2>-1)&&(col-1>-1))
                    {
                        if(led[lin+2][col-1]>-1&&led[lin+2][col-1]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+2][col-1]);
                            led_strip_set_pixel(led_strip,led[lin+2][col-1],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin-2][col+1]==0)
                {
                    if ((lin-2<8)&&(col+1<8)&&(lin-2>-1)&&(col+1>-1))
                    {
                        if(led[lin-2][col+1]>-1&&led[lin-2][col+1]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-2][col+1]);
                            led_strip_set_pixel(led_strip,led[lin-2][col+1],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin-2][col+1]>96)&&(Tabuleiro[lin-2][col+1]<123))
                {
                    if ((lin-2<8)&&(col+1<8)&&(lin-2>-1)&&(col+1>-1))
                    {
                        if(led[lin-2][col+1]>-1&&led[lin-2][col+1]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-2][col+1]);
                            led_strip_set_pixel(led_strip,led[lin-2][col+1],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin-2][col-1]==0)
                {
                    if((lin-2<8)&&(col-1<8)&&(lin-2>-1)&&(col-1>-1))
                    {
                        if(led[lin-2][col-1]>-1&&led[lin-2][col-1]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-2][col-1]);
                            led_strip_set_pixel(led_strip,led[lin-2][col-1],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin-2][col-1]>96)&&(Tabuleiro[lin-2][col-1]<123))
                {
                    if((lin-2<8)&&(col-1<8)&&(lin-2>-1)&&(col-1>-1))
                    {
                        if(led[lin-2][col-1]>-1&&led[lin-2][col-1]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-2][col-1]);
                            led_strip_set_pixel(led_strip,led[lin-2][col-1],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin+1][col+2]==0)
                {
                    if((lin+1<8)&&(col+2<8)&&(lin+1>-1)&&(col+2>-1))
                    {
                        if(led[lin+1][col+2]>-1&&led[lin+1][col+2]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+1][col+2]);
                            led_strip_set_pixel(led_strip,led[lin+1][col+2],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin+1][col+2]>96)&&(Tabuleiro[lin+1][col+2]<123))
                {
                    if((lin+1<8)&&(col+2<8)&&(lin+1>-1)&&(col+2>-1))
                    {
                        if(led[lin+1][col+2]>-1&&led[lin+1][col+2]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+1][col+2]);
                            led_strip_set_pixel(led_strip,led[lin+1][col+2],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin-1][col+2]==0)
                {
                    if((lin-1<8)&&(col+2<8)&&(lin-1>-1)&&(col+2>-1))
                    {
                        if(led[lin-1][col+2]>-1&&led[lin-1][col+2]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-1][col+2]);
                            led_strip_set_pixel(led_strip,led[lin-1][col+2],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin-1][col+2]>96)&&(Tabuleiro[lin-1][col+2]<123))
                {
                    if((lin-1<8)&&(col+2<8)&&(lin-1>-1)&&(col+2>-1))
                    {
                        if(led[lin-1][col+2]>-1&&led[lin-1][col+2]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-1][col+2]);
                            led_strip_set_pixel(led_strip,led[lin-1][col+2],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin+1][col-2]==0)
                {
                    if((lin+1<8)&&(col-2<8)&&(lin+1>-1)&&(col-2>-1))
                    {
                        if(led[lin+1][col-2]>-1&&led[lin+1][col-2]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+1][col-2]);
                            led_strip_set_pixel(led_strip,led[lin+1][col-2],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin+1][col-2]>96)&&(Tabuleiro[lin+1][col-2]<123))
                {
                    if((lin+1<8)&&(col-2<8)&&(lin+1>-1)&&(col-2>-1))
                    {
                        if(led[lin+1][col-2]>-1&&led[lin+1][col-2]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+1][col-2]);
                            led_strip_set_pixel(led_strip,led[lin+1][col-2],255,0,0);
                        }
                    }
                }
                if (Tabuleiro[lin-1][col-2]==0)
                {
                    if((lin-1<8)&&(col-2<8)&&(lin-1>0)&&(col-2>-1))
                    {
                        if(led[lin-1][col-2]>-1&&led[lin-1][col-2]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-1][col-2]);
                            led_strip_set_pixel(led_strip,led[lin-1][col-2],0,0,255);
                        }
                    }
                } else if ((Tabuleiro[lin-1][col-2]>96)&&(Tabuleiro[lin-1][col-2]<123))
                {
                    if((lin-1<8)&&(col-2<8)&&(lin-1>-1)&&(col-2>-1))
                    {
                        if(led[lin-1][col-2]>-1&&led[lin-1][col-2]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-1][col-2]);
                            led_strip_set_pixel(led_strip,led[lin-1][col-2],255,0,0);
                        }
                    }
                }
            break;
            case 'B': // Bispo branco
                for (i=1;i<8;i++)
                {
                    if ((lin-i<0)||(col-i<0)||(lin-i>7)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col-i]==0) //diagonal esquerda inferior
                    {
                        if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin-i][col-i]>96)&&(Tabuleiro[lin-i][col-i]<123))
                    {
                        if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col-i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin-i][col-i]>65)&&(Tabuleiro[lin-i][col-i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin+i<0)||(col-i<0)||(lin+i>7)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col-i]==0) //diagnoal esquerda superior
                    {
                        if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin+i][col-i]>96)&&(Tabuleiro[lin+i][col-i]<123))
                    {
                        if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col-i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin+i][col-i]>65)&&(Tabuleiro[lin+i][col-i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin+i<0)||(col+i<0)||(lin+i>7)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col+i]==0) //diagonal direita superior
                    {
                        if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col+i],0,0,255);
                        }
                    }else if ((Tabuleiro[lin+i][col+i]>96)&&(Tabuleiro[lin+i][col+i]<123))
                    {
                        if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col+i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin+i][col+i]>65)&&(Tabuleiro[lin+i][col+i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin-i<0)||(col+i<0)||(lin-i>7)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col+i]==0) //diagonal direita inferior
                    {
                        if(led[lin-i][col+i]>-1&&led[lin-i][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col+i],0,0,255);
                        }
                    }else if ((Tabuleiro[lin-i][col+i]>96)&&(Tabuleiro[lin-i][col+i]<123))
                    {
                        if(led[lin-i][col+i]>-1&&led[lin-i][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col+i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin-i][col+i]>65)&&(Tabuleiro[lin-i][col+i]<91))
                    {
                        break;
                    }
                }
            break;
            case 'D': // Dama branca
                for (i=1;i<8;i++)
                {
                    if ((lin-i<0)||(col-i<0)||(lin-i>7)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col-i]==0) //diagonal esquerda inferior
                    {
                        if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin-i][col-i]>96)&&(Tabuleiro[lin-i][col-i]<123))
                    {
                        if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col-i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin-i][col-i]>65)&&(Tabuleiro[lin-i][col-i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin+i<0)||(col-i<0)||(lin+i>7)||(col-i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col-i]==0) //diagnoal esquerda superior
                    {
                        if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin+i][col-i]>96)&&(Tabuleiro[lin+i][col-i]<123))
                    {
                        if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col-i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col-i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin+i][col-i]>65)&&(Tabuleiro[lin+i][col-i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin+i<0)||(col+i<0)||(lin+i>7)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col+i]==0) //diagonal direita superior
                    {
                        if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col+i],0,0,255);
                        }
                    }else if ((Tabuleiro[lin+i][col+i]>96)&&(Tabuleiro[lin+i][col+i]<123))
                    {
                        if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin+i][col+i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin+i][col+i]>65)&&(Tabuleiro[lin+i][col+i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<8;i++)
                {
                    if ((lin-i<0)||(col+i<0)||(lin-i>7)||(col+i>7))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col+i]==0) //diagonal direita inferior
                    {
                        if(led[lin-i][col+i]>-1&&led[lin-i][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col+i],0,0,255);
                        }
                    }else if ((Tabuleiro[lin-i][col+i]>96)&&(Tabuleiro[lin-i][col+i]<123))
                    {
                        if(led[lin-i][col+i]>-1&&led[lin-i][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col+i]);
                            led_strip_set_pixel(led_strip,led[lin-i][col+i],255,0,0);
                        }
                        break;
                    } else if ((Tabuleiro[lin-i][col+i]>65)&&(Tabuleiro[lin-i][col+i]<91))
                    {
                        break;
                    }
                }
                for(i=1;i<(col);i++)
                {
                    if (i==8||(col-i<0))
                    {
                        break;
                    }
                    if (Tabuleiro[lin][col-i]==0) //esquerda
                    {
                        if((led[lin][col-i]>-1)&&(led[lin][col-i]<64))
                        {
                            printf("Led a acender: %d Azul\n", led[lin][col-i]);
                            led_strip_set_pixel(led_strip,led[lin][col-i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin][col-i]>96)&&(Tabuleiro[lin][col-i]<123))
                    {
                        if(led[lin][col-i]>-1&&led[lin][col-i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin][col-i]);
                            led_strip_set_pixel(led_strip,led[lin][col-i],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin][col-i]>65)&&(Tabuleiro[lin][col-i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-lin);i++)
                {
                    if (i==8||(lin+i<0))
                    {
                        break;
                    }
                    if (Tabuleiro[lin+i][col]==0) //acima
                    {
                        if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin+i][col]);
                            led_strip_set_pixel(led_strip,led[lin+i][col],0,0,255);
                        }
                    } else if ((Tabuleiro[lin+i][col]>96)&&(Tabuleiro[lin+i][col]<123))
                    {
                        if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin+i][col]);
                            led_strip_set_pixel(led_strip,led[lin+i][col],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin+i][col]>65)&&(Tabuleiro[lin+i][col]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-col);i++)
                {
                    if (i==8||(col+i<0))
                    {
                        break;
                    }
                    if (Tabuleiro[lin][col+i]==0) //direita
                    {
                        if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin][col+i]);
                            led_strip_set_pixel(led_strip,led[lin][col+i],0,0,255);
                        }
                    } else if ((Tabuleiro[lin][col+i]>96)&&(Tabuleiro[lin][col+i]<123))
                    {
                        if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin][col+i]);
                            led_strip_set_pixel(led_strip,led[lin][col+i],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin][col+i]>65)&&(Tabuleiro[lin][col+i]<91))
                    {
                        break;
                    }
                }
                for (i=1;i<(9-lin);i++)
                {
                    if (i==8||(lin-i<0))
                    {
                        break;
                    }
                    if (Tabuleiro[lin-i][col]==0) //abaixo
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("Led a acender: %d Azul\n", led[lin-i][col]);
                            led_strip_set_pixel(led_strip,led[lin-i][col],0,0,255);
                        }
                    } else if ((Tabuleiro[lin-i][col]>96)&&(Tabuleiro[lin-i][col]<123))
                    {
                        if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                        {
                            printf("Led a acender: %d Red\n", led[lin-i][col]);
                            led_strip_set_pixel(led_strip,led[lin-i][col],255,0,0);
                            break;
                        }
                    } else if ((Tabuleiro[lin-i][col]>65)&&(Tabuleiro[lin-i][col]<91))
                    {
                        break;
                    }
                }
            break;
            case 'R': // Rei branco
                i=1;
                if (Tabuleiro[lin-i][col-i]==0) //diagonal esquerda inferior
                {
                    if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin-i][col-i]);
                        led_strip_set_pixel(led_strip,led[lin-i][col-i],0,0,255);
                    }
                } else if ((Tabuleiro[lin-i][col-i]>96)&&(Tabuleiro[lin-i][col-i]<123))
                {
                    if(led[lin-i][col-i]>-1&&led[lin-i][col-i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin-i][col-i]);
                        led_strip_set_pixel(led_strip,led[lin-i][col-i],255,0,0);
                    }
                }
                if (Tabuleiro[lin][col-i]==0) //esquerda
                {
                    if(led[lin][col-i]>-1&&led[lin][col-i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin][col-i]);
                        led_strip_set_pixel(led_strip,led[lin][col-i],0,0,255);
                    }
                } else if ((Tabuleiro[lin][col-i]>96)&&(Tabuleiro[lin][col-i]<123))
                {
                    if(led[lin][col-i]>-1&&led[lin][col-i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin][col-i]);
                        led_strip_set_pixel(led_strip,led[lin][col-i],255,0,0);
                    }
                }
                if (Tabuleiro[lin+i][col-i]==0) //diagnoal esquerda superior
                {
                    if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin+i][col-i]);
                        led_strip_set_pixel(led_strip,led[lin+i][col-i],0,0,255);
                    }
                } else if ((Tabuleiro[lin+i][col-i]>96)&&(Tabuleiro[lin+i][col-i]<123))
                {
                    if(led[lin+i][col-i]>-1&&led[lin+i][col-i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin+i][col-i]);
                        led_strip_set_pixel(led_strip,led[lin+i][col-i],255,0,0);
                    }
                }
                if (Tabuleiro[lin+i][col]==0) //acima
                {
                    if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin+i][col]);
                        led_strip_set_pixel(led_strip,led[lin+i][col],0,0,255);
                    }
                } else if ((Tabuleiro[lin+i][col]>96)&&(Tabuleiro[lin+i][col]<123))
                {
                    if(led[lin+i][col]>-1&&led[lin+i][col]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin+i][col]);
                        led_strip_set_pixel(led_strip,led[lin+i][col],255,0,0);
                    }
                }
                if (Tabuleiro[lin+i][col+i]==0) //diagonal direita superior
                {
                    if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin+i][col+i]);
                        led_strip_set_pixel(led_strip,led[lin+i][col+i],0,0,255);
                    }
                } else if ((Tabuleiro[lin+i][col+i]>96)&&(Tabuleiro[lin+i][col+i]<123))
                {
                    if(led[lin+i][col+i]>-1&&led[lin+i][col+i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin+i][col+i]);
                        led_strip_set_pixel(led_strip,led[lin+i][col+i],255,0,0);
                    }
                }
                if (Tabuleiro[lin][col+i]==0) //direita
                {
                    if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin][col+i]);
                        led_strip_set_pixel(led_strip,led[lin][col+i],0,0,255);
                    }
                } else if ((Tabuleiro[lin][col+i]>96)&&(Tabuleiro[lin][col+i]<123))
                {
                    if(led[lin][col+i]>-1&&led[lin][col+i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin][col+i]);
                        led_strip_set_pixel(led_strip,led[lin][col+i],255,0,0);
                    }
                }
                if (Tabuleiro[lin-1][col+i]==0) //diagonal direita inferior
                {
                    if(led[lin-1][col+i]>-1&&led[lin-1][col+i]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin-1][col+i]);
                        led_strip_set_pixel(led_strip,led[lin-1][col+i],0,0,255);
                    }
                } else if ((Tabuleiro[lin-1][col+i]>96)&&(Tabuleiro[lin-1][col+i]<123))
                {
                    if(led[lin-1][col+i]>-1&&led[lin-1][col+i]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin-1][col+i]);
                        led_strip_set_pixel(led_strip,led[lin-1][col+i],255,0,0);
                    }
                }
                if (Tabuleiro[lin-i][col]==0) //abaixo
                {
                    if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                    {
                        printf("Led a acender: %d Azul\n", led[lin-i][col]);
                        led_strip_set_pixel(led_strip,led[lin-i][col],0,0,255);
                    }
                } else if ((Tabuleiro[lin-i][col]>96)&&(Tabuleiro[lin-i][col]<123))
                {
                    if(led[lin-i][col]>-1&&led[lin-i][col]<64)
                    {
                        printf("Led a acender: %d Red\n", led[lin-i][col]);
                        led_strip_set_pixel(led_strip,led[lin-i][col],255,0,0);
                    }
                }
            break;
        }
        linx = lin;
        colx = col;
        printf("Termiando movimento, esperando casa para onde vai\n");
    } else if (contr == 1)
    {
        led_strip_clear(led_strip);//apagar todos os leds
        led_strip_set_pixel(led_strip,led[linx][colx],255,255,255);//acender led de linx e colx (Casa antiga)
        led_strip_set_pixel(led_strip,led[lin][col],128,0,128);//acender led de lin e col em roxo para diferenciar (Casa Nova) 
        printf("O Led da peça é: %d em branco\n", led[linx][colx]);
        printf("O Led da casa escolhida é: %d em roxo\n", led[lin][col]);
        printf("Ligados os Led para confirmação\n");
    } else if (contr == 2)
    {
        led_strip_clear(led_strip);
        Tabuleiro[lin][col] = Tabuleiro[linx][colx]; // Coloca a peça na nova casa
        Tabuleiro[linx][colx] = 0; // Retira a peça da casa antiga
        printf("Peça atualizada\n");
    }
    led_strip_refresh(led_strip);
}

char Casa()
{
    switch(Jogada[0])
    {
        case 'a':
        case 'A':
            printf("Coluna A");
            coly = 0;
            liny = '8' - (Jogada[1]);
        break;
        case 'b':
        case 'B':
            printf("Coluna B");
            coly = 1;
            liny = '8' - Jogada[1];
        break;
        case 'c':
        case 'C':
            printf("Coluna C");
            coly = 2;
            liny = '8' - Jogada[1];
        break;
        case 'd':
        case 'D':
            printf("Coluna D");
            coly = 3;
            liny = '8' - Jogada[1];
        break;
        case 'e':
        case 'E':
            printf("Coluna E");
            coly = 4;
            liny = '8' - Jogada[1];
        break;
        case 'f':
        case 'F':
            printf("Coluna F");
            coly = 5;
            liny = '8' - Jogada[1];
        break;
        case 'g':
        case 'G':
            printf("Coluna G");
            coly = 6;
            liny = '8' - Jogada[1];
        break;
        case 'h':
        case 'H':
            printf("Coluna H");
            coly = 7;
            liny = '8' - Jogada[1];
        break;
    }
    printf("\nA peça eh: %c", Tabuleiro[liny][coly]);
    printf("\nO Led eh: %d\n", led[liny][coly]);
    return (Tabuleiro[liny][coly]);
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id ;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "Tabuleiro", "Bem-Vindo", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id = %d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, "Jogadas", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id = %d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "Tabuleiro", "Inicio da Partida", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC = %.*s\r\n", event->topic_len, event->topic);
        printf("DATA = %.*s\r\n", event->data_len, event->data);
        strncpy(Jogada, event->data, sizeof(Jogada));
        Jogada[2] = 0;
        printf("%s\n", Jogada);
        if ((((Jogada[0]>64)&&(Jogada[0]<73))||((Jogada[0]>96)&&(Jogada[0]<105)))&&(Jogada[1]>48)&&(Jogada[1]<57))
        {
            if(contr==0)
            {
                conf = Casa();
                if(conf!=0)
                {
                    if(((cor%2)==1)&&(conf>65&&conf<91))
                    {
                        Movimento(liny, coly);
                        contr = 1;
                    } else if(((cor%2)==0)&&(conf>96&&conf<123))
                    {
                        Movimento(liny, coly);
                        contr = 1;
                    } else 
                    {
                        contr = 0;
                        printf("Peça errada, não é sua vez de jogar\n");
                    }
                }
            } else if(contr == 1)
            {
                Casa();
                Movimento(liny, coly);
                contr = 2;
            }
        } else if(((Jogada[0]=='o')||(Jogada[0]=='O'))&&((Jogada[1]=='k')||(Jogada[1]=='K'))&&(contr==2))
        {
            Movimento(liny,coly); //Casa digitada anteriomente
            contr = 0;
            cor++;
            led_strip_clear(led_strip);
            printf("Tudo pronto\n");
        } else if(((Jogada[0]=='n')||(Jogada[0]=='N'))&&((Jogada[1]=='o')||(Jogada[1]=='O')))
        {
            contr = 0;
            led_strip_clear(led_strip);
            printf("Movimento Cancelado\n");
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("capturedl\n as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    configure_led();
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}