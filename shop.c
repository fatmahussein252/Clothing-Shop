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

void *
customer_thread (void *products)
{
  int i;
  Product *stored_products = (Product *) products;
  struct mq_attr attr;
  // open mq from customer to shop 
  mqd_t customer_mq = mq_open ("/customer_shop_mq", O_RDWR);
  
  // open mq from shop to customer 
  mqd_t shop_mq;
  if ((shop_mq =
       mq_open ("/shop_customer_mq", O_CREAT | O_EXCL, S_IRWXU, NULL)) < 0
      && errno == EEXIST)
    {
      if ((shop_mq = mq_open ("/shop_customer_mq", O_RDWR)) < 0)
	{
	  printf ("%s", strerror (errno));
	  exit (1);
	}
    }
    // get default msg size
  if (mq_getattr (customer_mq, &attr) == -1)
    {
      perror ("mq_getattr: ");
      exit (1);
    }
  // allocate pointer for recieving customer order
  Product *customer_msg = malloc (attr.mq_msgsize);
  if (!customer_msg)
    {
      perror ("malloc: ");
      exit (1);
    }

  // allocate pointer for recieving customer order
  char *shop_msg = malloc (100);
  if (!shop_msg)
    {
      perror ("malloc: ");
      exit (1);
    }
    
  sem_t *shop_sem = sem_open("/shop_sem", O_CREAT,
                       S_IRWXU, 1);
  while (1)
    {
	// revieve order
      if ((mq_receive
	   (customer_mq, (char *) customer_msg, attr.mq_msgsize, NULL)) < 0
	  && errno != EAGAIN)
	perror ("recieve: ");
	
     
	// check for valid order
      for (i = 0; i < PRODUCTS_NUM; i++)
	{
	   sem_wait(shop_sem);
	  if (strcmp (stored_products[i].name, customer_msg->name) == 0)
	    {
	      if (stored_products[i].count >= customer_msg->count)
		{
		  stored_products[i].count -= customer_msg->count;
		  sem_post(shop_sem);
		  strcpy(shop_msg, "Order reserved successfully :)\n");
		  if (mq_send
		      (shop_mq, shop_msg, strlen(shop_msg),
		       0) < 0)
		    {
		      printf ("%s", strerror (errno));
		      exit (1);
		    }
		    break;
		}
	      else
		{
		  sem_post(shop_sem);
		  strcpy(shop_msg, "The count ordered is not available :(\n");
		  if (mq_send
		      (shop_mq, shop_msg,
		       strlen(shop_msg), 0) < 0)
		    {
		      printf ("%s", strerror (errno));
		      exit (1);
		    }
		    break;
		}

	    }

	
	}
	
	if (i == PRODUCTS_NUM){
	sem_post(shop_sem);
	strcpy(shop_msg, "The type ordered is not available :(\n");
	  if (mq_send
	      (customer_mq, shop_msg,
	       sizeof(shop_msg), 0) < 0)
	    {
	      printf ("%s", strerror (errno));
	      exit (1);
	    }
	
	
	}
    }

}

int
main ()
{

  int storage_fd;
  ssize_t r_size;
  size_t file_size;
  char r_buf[100];
  mqd_t manager_mq;
  Product products_init[] = {
    5, "dress", 700,
    7, "skirt", 300,
    3, "T-shirt", 400
  };



  // open the storage file for mapping
  storage_fd = open ("./storage.txt", O_RDWR);
  if (storage_fd < 0)
    {
      perror ("open: ");
      exit (1);
    }

  write (storage_fd, products_init, sizeof (products_init));


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

	
   // create thread for handling customer orders in parallel
  pthread_t customer_tid;
  if (pthread_create (&customer_tid, NULL, customer_thread, (void *) products)
      < 0)
    {
      perror ("pthread_create: ");
      exit (1);
    }




  // opening message queue for communication with the manager
  if ((manager_mq =
       mq_open ("/manager_shop_mq", O_CREAT | O_EXCL, S_IRWXU, NULL)) < 0
      && errno == EEXIST)
    {
      if ((manager_mq = mq_open ("/manager_shop_mq", O_RDWR)) < 0)
	{
	  printf ("%s", strerror (errno));
	  exit (1);
	}
    }
    
  sem_t *shop_sem = sem_open("/shop_sem", O_CREAT,
                       S_IRWXU, 1);

  int i;
  while (1)
    {
      
      
      for (i = 0; i < PRODUCTS_NUM; i++)
	{
	  sem_wait(shop_sem);
	  printf
	    ("Available items:\n	Type: %s\n	price: %f\n	Available count: %i\n",
	     products[i].name, products[i].price, products[i].count);

	  if (products[i].count == 0)
	    {
	      
	      if (mq_send
		  (manager_mq, products[i].name, strlen (products[i].name),
		   0) < 0)
		{
		  printf ("%s", strerror (errno));
		  exit (1);
		}

	    }
	  sem_post(shop_sem);
	}
	
      sleep (2);
    }

  // Unmap memory before exiting
  munmap (products, file_size);


}
