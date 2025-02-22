#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <mqueue.h>
#include <errno.h>
#define PRODUCTS_NUM 3
struct Product
{
  int count;
  char name[20];
  float price;

} __attribute__((packed));


int
main ()
{
  mqd_t customer_mq;
  struct mq_attr attr;



  // open message queue from customer to shop
  if ((customer_mq =
       mq_open ("/customer_shop_mq", O_CREAT | O_EXCL, S_IRWXU, NULL)) < 0
      && errno == EEXIST)
    {
      if ((customer_mq = mq_open ("/customer_shop_mq", O_RDWR)) < 0)
	{
	  printf ("%s", strerror (errno));
	  exit (1);
	}
    }

  if (mq_getattr (customer_mq, &attr) == -1)
    {
      perror ("mq_getattr: ");
      exit (1);
    }

  // open mq from customer to shop 
  mqd_t shop_mq = mq_open ("/shop_customer_mq", O_RDWR);
  
  // allocate pointer for recieving shop response
  char *shop_msg = malloc (100);
  if (!shop_msg)
    {
      perror ("malloc: ");
      exit (1);
    }
    
   // take order
  struct Product *order = malloc (sizeof (order));	// Allocate memory for order type and count
  if (!order)
    {
      perror ("malloc");
      exit (1);
    }
printf ("Welcome to the shop :)\n");
while(1){ 
  
  
  printf("Please Enter the product type: ");
  fgets (order->name, sizeof (order->name), stdin);
  if (order->name != NULL && order->name[strlen (order->name) - 1] == '\n')
    order->name[strlen (order->name) - 1] = '\0';


  printf ("Please Enter the count needed: ");
  order->count = getchar();
  order->count = atoi((char*)&order->count);

  // send order to shop
  if (mq_send (customer_mq, (char *) order, sizeof (struct Product), 0) < 0)
    {
      printf ("%s", strerror (errno));
      exit (1);
    }

   
    
    // revieve order
      if ((mq_receive
	   (shop_mq, shop_msg, attr.mq_msgsize, NULL)) < 0
	  && errno != EAGAIN)
	perror ("recieve: ");

	printf("%s", shop_msg);
	// Clear stdin before asking for the next input
  int c;
  while ((c = getchar()) != '\n' && c != EOF);
}
}


