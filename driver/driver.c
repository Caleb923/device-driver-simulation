
/**********************************************************************/
/*                                                                    */
/* Program Name: Driver                                               */
/* Author:       Caleb Patterson                                      */
/* Installation: Pensacola Christian College, Pensacola, Florida      */
/* Course:       CS326, Operating Systems                             */
/* Date Written: April  29, 2019                                       */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/*                                                                    */
/* I pledge  the C language  statements in  this  program are  my own */
/* original  work,  and none  of the  C language  statements in  this */
/* program were copied  from any one else,  unless I was specifically */
/* authorized to do so by my CS326 instructor.                        */
/*                                                                    */
/*                                                                    */
/*                           Signed: ________________________________ */
/*                                             (signature)            */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/
/*                                                                    */
/* This is a disk device driver which accepts read/write requests     */
/* from a FILE SYSTEM, translates them from physical block numbers    */
/* into disk drive cylinder, track, and sector numbers, then          */
/* instructs a disk device to carry out the read and write requests   */
/*                                                                    */
/**********************************************************************/

/**********************************************************************/

#include <stdio.h>    /* printf                                       */

/**********************************************************************/
/*                        Symbolic Constants                          */
/**********************************************************************/
#define SECTORS     9    /* Sectors per track                         */
#define TRACKS      2    /* Tracks per cylinder                       */
#define CYLINDERS   40   /* Cylinders per disk                        */
#define SECTOR_BYTE 512  /* Bytes per sector                          */
#define BLOCK_BYTE  1024 /* Bytes per block                           */
#define MAX_LIST    20   /* Max number of requests in list            */
#define TRUE        1    /* Value of true                             */
#define FALSE       0    /* Value of false                            */

/**********************************************************************/
/*                        Program Structures                          */
/**********************************************************************/
/* File system message                                                */
struct message
{
   int  operation_code;               /* The disk operation to be     */
                                      /* performed                    */
   int  request_number;               /* A unique request number      */
   int  block_number;                 /* The block number to be read  */ 
                                      /* or written                   */
   int  block_size ;                  /* The block size in bytes      */
   unsigned long int *p_data_address; /* Points to the data block in  */
                                      /* in memory                    */
};                                    
struct message fs_message[MAX_LIST];


/* Pending file system message                                        */
struct pending
{
   int  operation_code;               /* The disk operation to be     */
                                      /* performed                    */
   int  request_number;               /* A unique request number      */
   int  block_number;                 /* The block number to be read  */ 
                                      /* or written                   */
   int  block_size ;                  /* The block size in bytes      */
   int  real;                         /* Is message real              */
   unsigned long int *p_data_address; /* Points to the data block in  */
                                      /* in memory                    */
}; 
struct pending;

/**********************************************************************/
/*                       Function Prototypes                          */
/**********************************************************************/
void convert_block (int block, int *p_cylinder, int *p_track, 
                    int *p_sector);
   /* Convert block numbers into cylinder, track, and sector numbers  */
int disk_drive(int code, int arg1, int arg2, int arg3, 
               unsigned long int *arg4);
   /* Disk drive function                                             */
 struct message *send_message (struct message fs_message[]);
   /* Send message back to the file system                            */
void motor_on(int *p_motor_off);
   /* Turn the motor on                                               */
void idle_driver();
  /* Send idle message to the file system                             */
void fill_pending(struct pending pending_list[]);
   /* Fill pending list with new requests                             */
void initialize_pending(struct pending pending_list[]);
   /* Initialize the pending list to empty                            */
int amount_pending(struct pending pending_list[]);
   /* Count the number of requests in pending list                    */
int amount_message();
   /* Count the number of requests in the file system                 */
void sort_pending(struct pending pending_list[]);
   /* Sort the pending list into ascending order                      */
int process_request(struct pending pending_list[], int index2, 
                    int *p_error);
   /* Process the next request                                        */

void elevator_algorithm(struct pending pending_list[], int *p_index2, 
              int cylinder);
   /* Decide next request to process                                  */
void swap_requests(int index, struct pending pending_list[], 
                   int index2);
   /* Swap two requests                                               */
void delete_process(struct pending pending_list[], int index2);
   /* Delete a request                                                */
int check_error(struct pending pending_list[], int index2);
   /* Check for any errors                                            */

/**********************************************************************/
/*                          Main Function                             */
/**********************************************************************/
int main()
{
   int block,                             /* File system block number */
       counter,                           /* Counts through for loop  */
       current_cylinder = 0,              /* Cylinder the heads are on*/
       cylinder,                          /* Memory disk cylinder     */
       error,                             /* Error code               */
       idle = 0,                          /* Is driver idle           */
       index = 0,                         /* Index of the file system */
       index2=0,                          /* Index of pending list    */
       motor_off = TRUE,                  /* Is motor off             */
       sector,                            /* Sector of memory disk    */
       track;                             /* Track of memory disk     */
   struct pending pending_list[MAX_LIST]; /* List of pending requests */
   
   /* Initialize the pending list to empty                            */ 
   initialize_pending(pending_list);
   
   /* Send an idle request to the file system                         */
   idle_driver();
   
   /* Fill the pending list                                           */
   fill_pending(pending_list);
   
   /* Make sure motor is on and up to speed                           */
   motor_on(&motor_off);

   /* Loop processing file system requests                            */
   while(TRUE)
   {
      /* Check to see if driver idle                                  */
      if (fs_message[0].operation_code == 0 && 
          amount_pending(pending_list) == 0)
      {   
         idle++;  
         if (idle >= 2)
         {  
            if (motor_off == 0)
            {
               disk_drive(8, 0, 0, 0, 0);
               motor_off = 1;
            }
         }
         idle_driver();
         fill_pending(pending_list);
      }
      
      else
      {
         /* Turn motor on if off                                      */
         idle = 0;
         if(motor_off)
         {
            motor_on(&motor_off);
            current_cylinder = disk_drive (1, 0, 0 ,0 ,0);
         }
         
         /* Find next file system request to process                   */
         elevator_algorithm(pending_list, &index2, current_cylinder);
         index = process_request(pending_list, index2, &error);
         for(counter =0; counter <10; counter++)
       
         /* Send any errors back to the file system                    */
         if (error < 0)
         { 
            swap_requests(index, pending_list, index2);
            fs_message[0].operation_code = error;
            send_message(fs_message);
            elevator_algorithm(pending_list, &index2, current_cylinder);
            index = process_request(pending_list, index2, &error);
         }
         error = 0;

         /* Seek to the correct cylinder                               */
         convert_block(fs_message[index].block_number, &cylinder, 
                       &track, &sector);
         if (current_cylinder != cylinder)
            current_cylinder = disk_drive(2, cylinder, 0, 0, 0);
         while (current_cylinder != cylinder)
         {  
            while(disk_drive(9, 0, 0, 0, 0) != 0)   
            {
            }
            if(cylinder == 0)
            {  
            current_cylinder = 0;
            break;
            }
            current_cylinder = disk_drive(2, cylinder, 0, 0, 0);
         }
         
         /* Set and process the DMA chips                               */
         disk_drive (3, sector, track, fs_message[index].block_size, 
                     fs_message[index].p_data_address);
         block = fs_message[index].operation_code;
         while(disk_drive (block + 5, 0, 0, 0, 0) == -2)
         {
         }
         
         /* Send the correct message back to the file system            */
         swap_requests(index, pending_list, index2);
         if (error>=0)
            fs_message[0].operation_code = 0;
         send_message(fs_message);
         fill_pending(pending_list);
      }
   }

   return 0;
}

/**********************************************************************/
/*   Convert block numbers into cylinder, track, and sector numbers   */
/**********************************************************************/
void convert_block (int block, int *p_cylinder, int *p_track, 
                                                          int *p_sector)
{
   /* Calculate the cylinder number                                   */
   *p_cylinder = (block - 1) / SECTORS;
   
   /* Calculate the track number                                      */
   *p_track    = (block - 1) % SECTORS;
   if (*p_track <= (SECTORS/TRACKS) )
      *p_track = 0;
   else
      *p_track = 1;
   
   /* Calculate the sector number                                     */
   *p_sector = ((block - 1) % SECTORS) * 2;
   if (*p_sector > SECTORS)
      *p_sector = *p_sector - SECTORS;

   return;
}

/**********************************************************************/
/*                   Turn the  motor on                               */
/**********************************************************************/
void motor_on(int *p_motor_off)
{
   int correct_speed = TRUE; /* Is motor correct speed                */  
   
   /* Turn motor on and wait to reach correct speed                   */
   if (disk_drive(4, 0, 0, 0, 0))
   {
      while (correct_speed != 0)
         correct_speed = disk_drive(5, 0, 0, 0, 0);
   }
   *p_motor_off = 0;
   return;
}

/**********************************************************************/
/*             Send idle message to the file system                   */
/**********************************************************************/
void idle_driver()
{
   int index = 0;   

   fs_message[index].operation_code = 
   fs_message[index].request_number = 
   fs_message[index].block_number = 
   fs_message[index].block_size = 0;
   fs_message[index].p_data_address = NULL;
   send_message(fs_message);
   return;
}

/**********************************************************************/
/*          Fill the pending list with file system requests           */
/**********************************************************************/
void fill_pending(struct pending pending_list[])
{
   int copy = TRUE,    /* Should request be copied                    */
       inner,          /* Counts through inner loop                   */
       number_pending, /* Number of pending requests                  */
       outer;          /* Counts through outer loop                   */
   
   number_pending = amount_pending(pending_list);
   
   /* See if request is in file system and pending list               */
   for (outer=0; outer < amount_message(); outer++)
   {
      for (inner=0; inner < number_pending; inner++)
      {
         if (fs_message[outer].request_number == 
             pending_list[inner].request_number)
         {
            copy = 0;
            break;
         }
         else
            copy = 1;
       }
      /* Copy file system request to pending list                     */ 
      if (copy == 1)
      {
         pending_list[number_pending].operation_code = 
                                       fs_message[outer].operation_code;
         pending_list[number_pending].request_number = 
                                       fs_message[outer].request_number;
         pending_list[number_pending].block_number   = 
                                         fs_message[outer].block_number;
         pending_list[number_pending].block_size     = 
                                           fs_message[outer].block_size;
         pending_list[number_pending].p_data_address = 
                                       fs_message[outer].p_data_address;
         pending_list[number_pending].real = 1;
         number_pending++;
      }
   }
   sort_pending(pending_list);

   return;
}

/**********************************************************************/
/*             Initialize the pending list to empty                   */
/**********************************************************************/
void initialize_pending(struct pending pending_list[])
{
   int counter; /* Count through loop                                 */

   for(counter = 0; counter < 20; counter++)
      pending_list[counter].real = 0;
   return;
}

/**********************************************************************/
/*                   Count the pending requests                       */
/**********************************************************************/
int amount_pending(struct pending pending_list[])
{
   int counter,            /* Count through the loop                  */
       number_pending = 0; /* Number of pending requests              */

   for(counter = 0; counter < 20; counter++)
   {
      if(pending_list[counter].real == 1)
         number_pending++;
   }

   return number_pending;
}

/**********************************************************************/
/*               Count the file system requests                       */
/**********************************************************************/
int amount_message()
{
   int counter,            /* Count through the loop                  */          
       number_pending = 0; /* Number of pending requests              */

   for(counter = 0; counter < 20; counter++)
   {
      if(fs_message[counter].operation_code != 0)
         number_pending++;
      else
         break;
   }

   return number_pending;
}

/**********************************************************************/
/*   Sort the pending requests into ascending order                   */
/**********************************************************************/
void sort_pending(struct pending pending_list[])
{
   int counter,         /* Counts through the outer loop              */
       index;           /* Index of every process                     */
   struct pending temp; /* Temporarily holds pending structure        */
   

   /* Sort the requests into ascending cylinder order                 */
   for(counter=1; counter<amount_pending(pending_list); counter++)
      for(index=0; index < (amount_pending(pending_list) - counter); 
                                                                index++)
      {
         if( (pending_list[index].block_number-1 / SECTORS) > 
             (pending_list[index + 1].block_number-1 / SECTORS))
         {
            temp = pending_list[index];
            pending_list[index] = pending_list[index+1];
            pending_list[index+1] = temp;
         } 
      }
   
return;
}

/**********************************************************************/
/*   Process request form pending list to file system                 */
/**********************************************************************/
int process_request(struct pending pending_list[], int index2, 
                    int *p_error)
{
   int index; /* Index of every file system message                   */
   
   /* Find same file system request as pending request                */
   for(index=0; index < 20; index++)
      if (fs_message[index].request_number == 
          pending_list[index2].request_number)
      {  
         break;
      }
    
    /* If pending list request does not exist in file system swap it  */
    /* to the file sytem list                                         */   
    if (index == 20)
    {
       for(index = 0; index <20; index++)
       {
          if (fs_message[index].operation_code == 0)
          break;
       }
       fs_message[index].operation_code = 
                                     pending_list[index2].operation_code;
       fs_message[index].request_number = 
                                     pending_list[index2].request_number;
       fs_message[index].block_number   = 
                                       pending_list[index2].block_number;
       fs_message[index].block_size     = 
                                         pending_list[index2].block_size;
       fs_message[index].p_data_address = 
                                     pending_list[index2].p_data_address;
      }
   
   *p_error = check_error(pending_list, index2);
   fs_message[index].operation_code = pending_list[index2].operation_code;
   return index;
}

/**********************************************************************/
/*   Elevator algorithm deciding which request to process next        */
/**********************************************************************/
void elevator_algorithm(struct pending pending_list[], int *p_index2, 
   int cylinder)
{
   int greater = FALSE;

   /* Find the next highest cylinder in the list                      */
   for(*p_index2 = 0; *p_index2 < amount_pending(pending_list); 
                                                          (*p_index2)++)
      if((pending_list[*p_index2].block_number - 1 )/ SECTORS >= 
          cylinder) 
      {
         greater = TRUE;
         break;
      }

   if(greater == 0)
      *p_index2 = 0;
   return;
}

/**********************************************************************/
/*                         Swap two requests                          */
/**********************************************************************/
void swap_requests(int index, struct pending pending_list[], int index2)
{
   struct message temp; /* Temporarily holds message structure        */

   temp = fs_message[index];
   fs_message[index] = fs_message[0];
   fs_message[0] = temp;
   delete_process(pending_list, index2);
   return;
}

/**********************************************************************/
/*               Delete a process from the list                       */
/**********************************************************************/
void delete_process(struct pending pending_list[], int index2)
{
   struct pending temp; /* Temporarily holds pending structure        */
   
   for(; index2 < amount_pending(pending_list)-1;index2++)
      {
      temp = pending_list[index2];
      pending_list[index2] = pending_list[index2 + 1];
      pending_list[index2 + 1] = temp;
      }
   pending_list[index2].real = 0;
   return;
}

/**********************************************************************/
/*               Check the request for any errors                     */
/**********************************************************************/
int check_error(struct pending pending_list[], int index2)
{
   int error_code = 0;

   if(pending_list[index2].operation_code < 0 || 
      pending_list[index2].operation_code > 2)
      error_code += -1;
   if(pending_list[index2].request_number <= 0)
      error_code += -2;
   if(pending_list[index2].block_number < 1 || 
      pending_list[index2].block_number > SECTORS * TRACKS * CYLINDERS *
      SECTOR_BYTE / BLOCK_BYTE)
      error_code += -4;
   if(pending_list[index2].block_size <0 || pending_list[index2].block_size 
      > SECTORS * TRACKS * CYLINDERS * SECTOR_BYTE/CYLINDERS || 
      pending_list[index2].block_size % 2 !=0)
      error_code += -8;
   if(*pending_list[index2].p_data_address < 0)
      error_code += -16;

   return error_code;
}
