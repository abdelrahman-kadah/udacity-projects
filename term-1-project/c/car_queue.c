#include <stdbool.h>
#include <stdio.h>
#include "car_queue.h"

void initQueue(carQueue_S *q)
{
    q->size = 0;
    q->front = 0;
    q->rear = -1;
    for(char i = 0; i < DIR_COUNT; i++)
    {
      q->directionCount[i] = 0;
    }
    // Init all other queue variables
}

bool isQueueFull(carQueue_S *q)
{
  if(q->size == CAR_QUEUE_CAPACITY)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool isQueueEmpty(carQueue_S *q)
{
  if(q->size == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool front(carQueue_S* q, car_S* car) {
    if (isQueueEmpty(q)) {
        return false;
    }

    car->direction = q->cars[q->front].direction;

    return true;
}

bool enqueue(carQueue_S *q, car_S car)
{
    if(isQueueFull(q) || isQueueDirFull(q, car.direction))
    {
      return false;
    }
    
    q->rear++;
    q->cars[q->rear].direction = car.direction;
    q->directionCount[car.direction]++;
    q->size++;

    return true;
}

bool dequeue(carQueue_S *q, car_S *carOut)
{
    if(isQueueEmpty(q))
    {
      return false;
    }

    carOut->direction = q->cars[q->front].direction;
    if(q->directionCount[q->cars[q->front].direction] > 0)
    {
      q->directionCount[q->cars[q->front].direction]--;
    }
    q->front++;
    q->size--;
}

bool isQueueDirFull(carQueue_S *q, direction_E dir)
{
  if(q->directionCount[dir] > 2)
  {
    printf("Direction is full.\n");
    return true;
  }
  else
  {
    return false;
  }
}

void printQueue(const carQueue_S *q)
{
    printf("Queue: ");

    if (q->size == 0)
    {
        printf("0\n");
        return;
    }

    int count = 0;
    int index = q->front;
    while (count < q->size)
    {
        direction_E dir = q->cars[index].direction;
        printf("%s ", DIRECTION_NAMES[dir]);

        index = (index + 1) % CAR_QUEUE_CAPACITY;
        count++;
    }
    printf("\n");
}