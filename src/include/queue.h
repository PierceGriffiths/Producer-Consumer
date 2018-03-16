typedef struct{
    unsigned front, back, size, capacity, *array;
}Queue;

Queue* createQueue(unsigned capacity);

void deleteQueue(Queue *q);

int isEmpty(Queue *q);

int isFull(Queue *q);

unsigned enqueue(Queue *q, unsigned num);

unsigned dequeue(Queue *q);
