#include "NRF.h"
#include "SPI.h"
#include "TIM2.h"

#define  TX_ADDR_WIDTH 3
#define  TX_PLOAD_WIDTH 4
//neu sua lai TX_PLOAD_WIDTH thi phai suu lai gia tri TX_ADDRESS
uint8_t TX_ADDRESS[TX_ADDR_WIDTH] = {0x00,0xDD,0xCC};
uint8_t ACK[1] = {123};

void NRF24_reset(uint8_t REG )
{
	uint8_t rx_addr_p0_def[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
	uint8_t rx_addr_p1_def[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
	uint8_t tx_addr_def[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
	CE_OFF();
  DelayMicro(5000); 
	
	if (REG == STATUS) 
	{
		NRF24_WriteReg(STATUS, 0x00);
	}
	
	else 	if (REG == FIFO_STATUS)
	{
		NRF24_WriteReg(FIFO_STATUS,0x11);
	}				
	
	else 
	{
		NRF24_WriteReg(CONFIG,0x08);
		NRF24_WriteReg(EN_AA, 0x3F);
		NRF24_WriteReg(EN_RXADDR, 0x03);
		NRF24_WriteReg(SETUP_AW, 0x03);
		NRF24_WriteReg(SETUP_RETR, 0x03);
		NRF24_WriteReg(RF_CH, 0x02);
		NRF24_WriteReg(RF_SETUP, 0x0E);
		NRF24_WriteReg(STATUS, 0x00);
		NRF24_WriteReg(OBSERVE_TX, 0x00);
		NRF24_WriteReg_Mutil(RX_ADDR_P0,rx_addr_p0_def,5);
		NRF24_WriteReg_Mutil(RX_ADDR_P1,rx_addr_p1_def,5);
		NRF24_WriteReg(RX_ADDR_P2, 0xC3);
		NRF24_WriteReg(RX_ADDR_P3, 0xC4);
		NRF24_WriteReg(RX_ADDR_P4, 0xC5);
		NRF24_WriteReg(RX_ADDR_P5, 0xC6);	
		NRF24_WriteReg_Mutil(TX_ADDR,tx_addr_def,5);
		NRF24_WriteReg(RX_PW_P0, 0);
		NRF24_WriteReg(RX_PW_P1, 0);
		NRF24_WriteReg(RX_PW_P2, 0);
		NRF24_WriteReg(RX_PW_P3, 0);
		NRF24_WriteReg(RX_PW_P4, 0);
		NRF24_WriteReg(RX_PW_P5, 0);
		NRF24_WriteReg(FIFO_STATUS, 0x11);
		NRF24_WriteReg(DYNPD, 0);
		NRF24_WriteReg(FEATURE, 0);			
	}
}


void NRF24_Tx_ini(void)
{
	uint8_t config = 0;
	CE_OFF();
	config = NRF24L01P_Read_Register(CONFIG);
	config &= ~0x01;//PRIM_RX = 0:PTX
	NRF24_WriteReg (CONFIG, config);	//powper on 
	
	NRF24_reset(0);
	NRF24_WriteReg(EN_AA, 0x01);//unable auto ack on the data pipe 0
	NRF24_WriteReg(EN_RXADDR, 0);
	NRF24_WriteReg(SETUP_RETR, 0x05);//retransmit 5 time + wait 250 us
	NRF24_WriteReg(FEATURE, 0x02);//enable Payload with ACK
	NRF24_WriteReg(DYNPD, 0x01);//ennable dynamic payload length data pipe 0
	//use the same address with PRX
	NRF24_WriteReg(SETUP_AW, 0x01);// 3  Bytes for the TX/RX address
	NRF24_WriteReg_Mutil(TX_ADDR,TX_ADDRESS,TX_ADDR_WIDTH);
	NRF24_WriteReg_Mutil(RX_ADDR_P0,TX_ADDRESS,TX_ADDR_WIDTH);//use auto ack
	
	//use the same frequency =2476
	NRF24_WriteReg(RF_CH, 76);// period 2476 MHz	
	//select data rate
	NRF24_WriteReg(RF_SETUP, 0x02 );//TX_PWR:0dBm, Datarate:1Mbps
	//POWER UP 
	config = NRF24L01P_Read_Register(CONFIG);
	config |= 0x02;
	NRF24_WriteReg (CONFIG, config);	//powper on 
}

uint8_t NRF24_Transmit (uint8_t *data, uint8_t *ACK_set)
{
	uint8_t REG[1] = {WR_TX_PLOAD};//Write payload with ACK
	uint8_t ACK_recei[1];
	uint8_t fifostatus = 0;
	CE_OFF();
	DelayMicro(150);
	//SRRF[0]=NRF24L01P_Read_Register(FIFO_STATUS);
	CS_ON();
	SPI_Transmit(REG,1);//comand Write TX_PLD
	SPI_Transmit(data,TX_PLOAD_WIDTH);//Write Data
	
	//SRRF[1]=NRF24L01P_Read_Register(FIFO_STATUS);
	CS_OFF();
	CE_ON();
	fifostatus = NRF24L01P_Read_Register(FIFO_STATUS);
	//SRRF[2]=NRF24L01P_Read_Register(FIFO_STATUS);
	if (((fifostatus&0x20) != 0))//if TX FIFO full
			{	
				ACK_recei[0]=NRF24L01P_Read_Register(RD_RX_PLOAD);//Read Data Rx = ACK
				if (ACK_recei[0]==ACK_set[0])
				{	
				REG[0]=FLUSH_TX;
				SPI_Transmit(REG,1);		
				// reset FIFO_STATUS
				NRF24_reset (FIFO_STATUS); 
				//SRRF[3]=NRF24L01P_Read_Register(FIFO_STATUS);
				return 1;
				}
			}	 
	return 0;	
}
//uint8_t NRF24_Transmit (uint8_t *data)
//{
//	
//}

void NRF24_Rx_ini(void)
{
	uint8_t config = 0;
	CE_OFF();
	//select mode PRX
	config = NRF24L01P_Read_Register(CONFIG);
	config |= 0x01;
	NRF24_WriteReg (CONFIG, config);
	//reset NRF
	NRF24_reset(0);
	NRF24_reset(STATUS);
	
	NRF24_WriteReg(EN_RXADDR, 0x02);//enable data pipe 1
	NRF24_WriteReg(EN_AA, 0x01);//enable auto-ack pipe 1
	NRF24_WriteReg(DYNPD, 0x01);//ennable dynamic payload length data pipe 0
	
	NRF24_WriteReg(RX_PW_P1, TX_PLOAD_WIDTH);//  set numbers bit payload size for pipe 1 = TX_PLOAD_WIDTH
	NRF24_WriteReg_Mutil(RX_ADDR_P1, TX_ADDRESS,TX_ADDR_WIDTH);// Write the Pipe1 ADDR
		
	NRF24_WriteReg(SETUP_AW, 0x01);// 3 Bytes for the TX/RX address
	NRF24_WriteReg(SETUP_RETR, 0x05);//retransmit 5 time + wait 250 us
	NRF24_WriteReg(RF_CH, 76);// period 2476 MHz	
	NRF24_WriteReg(RF_SETUP, 0x02 );//TX_PWR:0dBm, Datarate:1Mbps
	NRF24_WriteReg(FEATURE, 0x06);//enable dynamic payload and Ack payload
	NRF24_WriteReg(DYNPD, 0x3F);//ennable dynamic payload length data pipe 0
	//NRF24_WriteReg(ACTIVATE,0x73);
	//power up
	config = NRF24L01P_Read_Register(CONFIG);
	config |= 0x02;
	NRF24_WriteReg (CONFIG, config);
}

void NRF24_Receive (uint8_t *data,uint8_t size)
{	
		
	//write ACK - TX_data
	uint8_t REG[1] = {WR_TX_PLOAD};
	CS_ON();
	SPI_Transmit(REG,1);
	SPI_Transmit(ACK, 1);
	CS_OFF();
	
	REG[0]=RD_RX_PLOAD;//set ADDR for receive
	
	CE_ON();
	DelayMicro(150);
	CE_OFF();
	NRF24L01P_MultiRead_Register(RD_RX_PLOAD,data,size);
	DelayMili(1);
	//dalay1
	REG[0]=FLUSH_RX;
	SPI_Transmit(REG, 1);	
	
}

void NRF24_WriteReg(uint8_t addr, uint8_t dt)
{
	uint8_t TxData[1];
	addr |= W_REGISTER;
  CS_ON();
	TxData[0]=addr;
	SPI_Transmit(TxData,1);
	TxData[0]=dt;
	SPI_Transmit(TxData,1);
  CS_OFF();
}

void NRF24_WriteReg_Mutil(uint8_t addr, uint8_t *dt, uint8_t size)
{
	uint8_t TxData[1];
	addr |= W_REGISTER;
  CS_ON();
	TxData[0]=addr;
	SPI_Transmit(TxData,1);
	SPI_Transmit(dt,size);
  CS_OFF();
}


//can tao mot point de luu gia tri doc RxData
void NRF24_Read_Buf(uint8_t addr,uint8_t *RxData,uint8_t bytes)
{
  uint8_t TxData[1];
	TxData[0] = addr;
	CS_ON();	
  SPI_Transmit(TxData,1);
 // SPI_Receiver(RxData,bytes);
  CS_OFF();
}

void NRF24_Write_Buf(uint8_t addr,uint8_t *pBuf,uint8_t bytes)
{
	uint8_t TxData[1];
	TxData[0] = addr|W_REGISTER;
  CS_ON();
	SPI_Transmit(TxData,1);
  DelayMicro(1);
  SPI_Transmit(pBuf,bytes);
  CS_OFF();
}
//------------------------------------------------


uint8_t NRF24L01P_Read_Register(uint8_t RegAdd){
	uint8_t READ_REG = 0;
	SPI2->CR1 |= SPI_CR1_SPE;
 	CS_ON();

	while(!(READ_BIT(SPI2->SR, SPI_SR_TXE) == (SPI_SR_TXE))) {}
	SPI2->DR = RegAdd;
	if(RegAdd == STATUS){
		while(!(READ_BIT(SPI2->SR, SPI_SR_RXNE) == (SPI_SR_RXNE))) {}
		READ_REG = SPI2->DR;
	}
	else{
		while(!(READ_BIT(SPI2->SR, SPI_SR_TXE) == (SPI_SR_TXE))) {}
		while (SPI2->SR & SPI_SR_BSY);
		SPI2->CR1 |= SPI_CR1_RXONLY;

		while(!(READ_BIT(SPI2->SR, SPI_SR_RXNE) == (SPI_SR_RXNE))) {}
		(void) SPI2->DR;


		while(!(READ_BIT(SPI2->SR, SPI_SR_RXNE) == (SPI_SR_RXNE))) {}
		READ_REG = SPI2->DR;
		CS_OFF();
		SPI2->CR1 &= ~SPI_CR1_RXONLY;
	}

	return READ_REG;

}


void NRF24L01P_MultiRead_Register(uint8_t RegAdd, uint8_t* Data, uint8_t size){
	uint8_t i = 0;
	SPI2->CR1 |= SPI_CR1_SPE;
	CS_ON();
	while(!(READ_BIT(SPI2->SR, SPI_SR_TXE) == (SPI_SR_TXE))) {}
	SPI2->DR = RegAdd;

	while(!(READ_BIT(SPI2->SR, SPI_SR_TXE) == (SPI_SR_TXE))) {}
	while (SPI2->SR & SPI_SR_BSY);
	SPI2->CR1 |= SPI_CR1_RXONLY;

	while(!(READ_BIT(SPI2->SR, SPI_SR_RXNE) == (SPI_SR_RXNE))) {}
	(void) SPI2->DR;

	for(i=0; i<size; i++){

		while(!(READ_BIT(SPI2->SR, SPI_SR_RXNE) == (SPI_SR_RXNE))) {}
		Data[i] = SPI2->DR;
	}

	CS_OFF();
	SPI2->CR1 &= ~SPI_CR1_RXONLY;
}



/*

//cai nay tu viet va chua dung toi 
//------------------------------------------------
//+++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++
void NRF24L01_RX_Mode(void)
{
	//set the PWR_UP and PRIM_RX bit to 1
  uint8_t regval=0x00;
  regval = NRF24_ReadReg(CONFIG);
  regval |= (1<<PWR_UP)|(1<<PRIM_RX);
  NRF24_WriteReg(CONFIG,regval);
	// Disable Pipe1
	NRF24_WriteReg(EN_AA, 0x00); 
	//Setup address width=3 bytes
	NRF24_WriteReg(SETUP_AW, 0x01);
	// f = 2476 MHz the same frequency chanel with Tx 
	NRF24_WriteReg(RF_CH, 76); 
	//set the data rate TX_PWR:0dBm, Datarate:1Mbps 
	NRF24_WriteReg(RF_SETUP, 0x06); 
  CE_ON();
  DelayMicro(150); //wait 130us
  // Flush buffers
  NRF24_FlushRX();
  NRF24_FlushTX();
}
//------------------------------------------------
void NRF24L01_TX_Mode (void)
{
	//set the PRIM_RX bit to 0
	uint8_t regval=0x00;
  regval = NRF24_ReadReg(CONFIG);
  regval &= ~(1<<PRIM_RX);
	NRF24_WriteReg(CONFIG,regval);
	//1500us, 15 retrans
	NRF24_WriteReg(SETUP_RETR, 0x5F); 
	//Setup address width=3 bytes
	NRF24_WriteReg(SETUP_AW, 0x01);
	// f = 2476 MHz the same frequency chanel with Tx 
	NRF24_WriteReg(RF_CH, 76); 
	//set the data rate TX_PWR:0dBm, Datarate:1Mbps 
	NRF24_WriteReg(RF_SETUP, 0x06); 
	//set the PWR_UP to high
  regval = NRF24_ReadReg(CONFIG);
  regval |= (1<<PWR_UP);
	NRF24_WriteReg(CONFIG,regval);
		
	CE_ON();//Pulse CE to transmit packet
  DelayMicro(150); //wait 130us
  // Flush buffers
  NRF24_FlushRX();
  NRF24_FlushTX();
}
//=====================================================================
//=====================================================================
//=====================================================================
void write_1_reg(uint8_t reg,uint8_t data)
{
	uint8_t addr=0;
	uint8_t tmp_over;
	addr |= (reg|W_REGISTER);
	CS_ON();
	SPI2->DR = addr;	
	while((SPI2->SR & SPI_SR_TXE)!=SPI_SR_TXE);
	SPI2->DR = (uint8_t)data;
	while((SPI2->SR & SPI_SR_TXE)!=SPI_SR_TXE);
	while((SPI2->SR & SPI_SR_BSY)==SPI_SR_BSY);
	tmp_over=SPI2->DR;
	tmp_over=SPI2->SR;
	CS_OFF();	
}


void read_1_reg(uint8_t reg,uint8_t *Rxdata)
{
	uint8_t addr=0;
	uint32_t tmp_over;
	addr = reg;
	CS_ON();
	SPI2->DR = addr;
	while((SPI2->SR & SPI_SR_TXE)!=SPI_SR_TXE);
	while((SPI2->SR & SPI_SR_BSY)==SPI_SR_BSY);
	tmp_over=SPI2->DR;
	tmp_over=SPI2->SR;
//	while ((SPI2->SR & SPI_SR_RXNE)!=SPI_SR_RXNE);
	SPI2->DR=0x00;
	while((SPI2->SR & SPI_SR_TXE)!=SPI_SR_TXE);
	while ((SPI2->SR & SPI_SR_RXNE)!=SPI_SR_RXNE);
	//SPI2->CR1 &= ~SPI_CR1_SPE;//Disable the SPI (SPE=0)
	Rxdata[0]=SPI2->DR;
	CS_OFF();	
}
//+++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++
*/

