#ifndef _UI_H_
#define _UI_H_

#define OLED_SPI_PIN_CLK                    GET_PIN(B, 1)
#define OLED_SPI_PIN_MOSI                   GET_PIN(H, 2)
#define OLED_SPI_PIN_RES                    GET_PIN(E, 6)
#define OLED_SPI_PIN_DC                     GET_PIN(E, 5)
#define OLED_SPI_PIN_CS                     GET_PIN(E, 4)

#define OLED_IIC_SDA                        GET_PIN(B, 1)
#define OLED_IIC_SCL                        GET_PIN(H, 2)

extern unsigned char qr_info[];

void oled_init(void);
void ui_init(void);
void oled_show_system_boot(char *str);


#endif
