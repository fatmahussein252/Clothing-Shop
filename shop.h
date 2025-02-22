
#define PRODUCTS_NUM 3
struct Product
{
  int count;
  char name[20];
  float price;

}__attribute__((packed));

typedef struct Product Product;
