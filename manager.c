#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <mqueue.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define PRODUCTS_NUM 3
struct Product
{
  int count;
  char name[20];
  float price;

}__attribute__((packed));

typedef struct Product Product;
void * user_thread(void *products)
{
	
	  int i;
	  struct Product *order = malloc (sizeof (order));	
	  if (!order)
	    {
	      perror ("malloc");
	      exit (1);
	    }
	  Product *stored_products = (Product *) products;
	  sem_t *shop_sem = sem_open("/shop_sem", O_CREAT,
                       S_IRWXU, 1);
	while(1){
	  printf("Enter the product type to change: ");
	  fgets (order->name, sizeof (order->name), stdin);
	  if (order->name != NULL && order->name[strlen (order->name) - 1] == '\n')
	    order->name[strlen (order->name) - 1] = '\0';
	  printf ("Enter the new count: ");
	  order->count = getchar();
	  order->count = atoi((char*)&order->count);
	  printf("Enter the new price: ");
	  scanf("%f", &order->price);

	for (i = 0; i < PRODUCTS_NUM; i++)
	{
	  sem_wait(shop_sem);
	  if (strcmp (stored_products[i].name, order->name) == 0)
	    {
	      stored_products[i].count = order->count;
	      stored_products[i].price = order->price;
	      sem_post(shop_sem);
	      break;
	    }
	    
	 }
	if (i == PRODUCTS_NUM){
		sem_post(shop_sem);
		printf("Invalid operation\n");
		}
		
	 int c;
  	while ((c = getchar()) != '\n' && c != EOF);
	}
	
	

}
int
main ()
{
  struct mq_attr attr;
  ssize_t rec_msg_len;
  int storage_fd;
  ssize_t r_size;
  size_t file_size;
  char r_buf[100];
  mqd_t manager_mq;
  /* openning file once for mapping */

  storage_fd = open ("./storage.txt", O_RDWR);
  if (storage_fd < 0)
    {
      perror ("open: ");
      exit (1);
    }


  // Get file size
  struct stat sb;
  if (fstat (storage_fd, &sb) == -1)
    {
      perror ("fstat");
      exit (1);
    }

  file_size = sb.st_size;

  // Map file to memory
  Product *products =
    mmap (NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, storage_fd, 0);
  if (products == MAP_FAILED)
    {
      perror ("mmap");
      exit (1);
    }

  close (storage_fd);		// fd is no longer needed after mmap
  
  // create thread for interacting with user in parallel.
   pthread_t customer_tid;
  if (pthread_create (&customer_tid, NULL, user_thread, (void *) products)
      < 0)
    {
      perror ("pthread_create: ");
      exit (1);
    }


  mqd_t shop_mq = mq_open ("/manager_shop_mq", O_RDONLY);

  if (mq_getattr (shop_mq, &attr) == -1)
    {
      perror ("mq_getattr: ");
      exit (1);
    }


  char *shop_msg = malloc (attr.mq_msgsize);
  if (!shop_msg)
    {
      perror ("malloc: ");
      exit (1);
    }
    
  sem_t *shop_sem = sem_open("/shop_sem", O_CREAT,
                       S_IRWXU, 1);
  while (1)
    {

      if ((rec_msg_len =
	   mq_receive (shop_mq, shop_msg, attr.mq_msgsize, NULL)) < 0
	  && errno != EAGAIN)
	perror ("recieve: ");

      for (int i = 0; i < PRODUCTS_NUM; i++)
	{
	  sem_wait(shop_sem);
	  if (strcmp (products[i].name, shop_msg) == 0)
	    products[i].count += 3;
	  sem_post(shop_sem);
	}
    }

}
