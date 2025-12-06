#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "car_queue.h"

//-------------------------------------------------------------------
// Defines and macros
//-------------------------------------------------------------------
// Outputs
#define EAST_R_PIN   (3U)
#define EAST_Y_PIN   (4U)
#define EAST_G_PIN   (5U)
#define NORTH_R_PIN  (6U)
#define NORTH_Y_PIN  (7U)
#define NORTH_G_PIN  (8U)
#define NORTH_A_PIN  (9U)
#define WEST_R_PIN  (18U)
#define WEST_Y_PIN  (17U)
#define WEST_G_PIN  (16U)
#define WEST_A_PIN  (21U)
#define NEW_CAR_PIN (15U)

//-------------------------------------------------------------------
// Enumerations and structs
//-------------------------------------------------------------------
typedef enum
{
  // Outputs
  GPIO_OUT_EAST_R,
  GPIO_OUT_EAST_Y,
  GPIO_OUT_EAST_G,
  GPIO_OUT_NORTH_R,
  GPIO_OUT_NORTH_Y,
  GPIO_OUT_NORTH_G,
  GPIO_OUT_NORTH_A,
  GPIO_OUT_WEST_R,
  GPIO_OUT_WEST_Y,
  GPIO_OUT_WEST_G,
  GPIO_OUT_WEST_A,
  GPIO_IN_NEW_CAR,
  GPIO_COUNT // Must be last
} gpioName_E;

// GPIO initialization table struct
typedef struct {
  uint pin;
  uint direction;
} gpioInit_S;

// State defintions enum
typedef enum
{
  STATE_S0,
  STATE_S1,
  STATE_S2,
  STATE_S3,
  STATE_S4,
  STATE_S5,
  STATE_S6,
  STATE_S7,
  // Add more states here

  STATE_COUNT // Must be last
} stateName_E;

// Output value struct
typedef struct {
  bool ER; // East Red
  bool EY; // East Yellow
  bool EG; // East Green
  bool NR; // North Red
  bool NY; // North Yellow
  bool NG; // North Green
  bool NA; // North Arrow
  bool WR; // West Red
  bool WY; // West Yellow
  bool WG; // West Green
  bool WA; // West Arrow
} stateOutputs_S;

//-------------------------------------------------------------------
// Global tables and data types
//-------------------------------------------------------------------

// OutputTable
// Each row contains a specific state for { ER,EY,EG,NR,NY,NG,NA,WR,WY,WG,WA }
// For example:
// All reds -> { true, false, false, true, false, false, false, true, false, false, false }
static const stateOutputs_S gOutputTable[STATE_COUNT] = {
    //     ER,    EY,    EG,    NR,    NY,    NG,    NA,    WR,    WY,    WG,    WA
    {false, false, true, true, false, false, false, false, false, true, false}, // S0
    {false, true, false, true, false, false, false, false, true, true, false},    // S1
    {true, false, false, true, false, false, true, false, false, true, true},
    {true, false, false, true, false, false, false, false, false, true, false},
    {false, true, false, true, false, false, false, false, true, false, false},
    {true, false, false, false, false, true, true, true, false, false, false},
    {true, false, false, false, true, false, false, true, false, false, false},
    {true, false, false, false, true, false, true, true, false, false, false},
};

static const gpioInit_S gpioList[GPIO_COUNT] = {
    {.pin = EAST_R_PIN, .direction = GPIO_OUT},
    {.pin = EAST_Y_PIN, .direction = GPIO_OUT},
    {.pin = EAST_G_PIN, .direction = GPIO_OUT},
    {.pin = NORTH_R_PIN, .direction = GPIO_OUT},
    {.pin = NORTH_Y_PIN, .direction = GPIO_OUT},
    {.pin = NORTH_G_PIN, .direction = GPIO_OUT},
    {.pin = NORTH_A_PIN, .direction = GPIO_OUT},
    {.pin = WEST_R_PIN, .direction = GPIO_OUT},
    {.pin = WEST_Y_PIN, .direction = GPIO_OUT},
    {.pin = WEST_G_PIN, .direction = GPIO_OUT},
    {.pin = WEST_A_PIN, .direction = GPIO_OUT},
    {.pin = NEW_CAR_PIN, .direction = GPIO_IN},
};

static stateName_E gState; // Main FSM state tracker
static uint32_t tick_count = 0;
static uint32_t traffic_tick = 0;

//-------------------------------------------------------------------
// Private function definitions
//-------------------------------------------------------------------

// This function initializes all used GPIOs in this project
static void gpioSetup(void)
{
  for (size_t i = 0U; i < GPIO_COUNT; i++)
  {
    // Initialize the GPIOs here
    gpio_init(gpioList[i].pin);
    if (GPIO_OUT == gpioList[i].direction)
    {
      gpio_set_dir(gpioList[i].pin, GPIO_OUT);
    }
    else
    {
      gpio_set_dir(gpioList[i].pin, GPIO_IN);
      gpio_pull_up(gpioList[i].pin);
    }
  }
}

//-------------------------------------------------------------------
// State machine framework functions
//-------------------------------------------------------------------
static void updateOutputs(const stateName_E currentState)
{
  if (currentState < STATE_COUNT)
  {
    stateOutputs_S *const pOutput = &gOutputTable[currentState];

    // Now update all the lights accordingly
    gpio_put(gpioList[GPIO_OUT_EAST_R].pin, pOutput->ER);
    gpio_put(gpioList[GPIO_OUT_EAST_Y].pin, pOutput->EY);
    gpio_put(gpioList[GPIO_OUT_EAST_G].pin, pOutput->EG);
    gpio_put(gpioList[GPIO_OUT_NORTH_R].pin, pOutput->NR);
    gpio_put(gpioList[GPIO_OUT_NORTH_Y].pin, pOutput->NY);
    gpio_put(gpioList[GPIO_OUT_NORTH_G].pin, pOutput->NG);
    gpio_put(gpioList[GPIO_OUT_NORTH_A].pin, pOutput->NA);
    gpio_put(gpioList[GPIO_OUT_WEST_R].pin, pOutput->WR);
    gpio_put(gpioList[GPIO_OUT_WEST_Y].pin, pOutput->WY);
    gpio_put(gpioList[GPIO_OUT_WEST_G].pin, pOutput->WG);
    gpio_put(gpioList[GPIO_OUT_WEST_A].pin, pOutput->WA);
  }
}

static void checkNewCarPressed(const carQueue_S *carQueue)
{
  static bool last_button_state = false;

  // False means pressed, true means unpressed
  bool current_button_state = !gpio_get(gpioList[GPIO_IN_NEW_CAR].pin);

  /*
   Check if the button is pressed or not

   Note: This must be a one time press!
   This means if I keep pressing the button
   it should only count as a single press.
   */

  if(last_button_state == false && current_button_state == true)
  {
    car_S car;
    car.direction = rand() % DIR_COUNT;
    enqueue((carQueue_S *)carQueue, car);
  }

  // printf("Last button state: %d\n", last_button_state);
  // printf("Current button state: %d\n", current_button_state);

  last_button_state = current_button_state;


}

static stateName_E getNextStateFromQueue(stateName_E currentState, const carQueue_S *carQueue)
{
 
  car_S car;

  if(front(carQueue, &car) == false)
  {
    return false;
  }
  
  switch (currentState)
  {
  case STATE_S0:
    if(car.direction == DIR_NORTH_L)
    {
      return STATE_S4;
    }
    else if(car.direction == DIR_NORTH_R || car.direction == DIR_WEST_L)
    {
      return STATE_S1;
    }
    else
    {
      return STATE_S0;
    }
  case STATE_S1:
    return STATE_S2;
  case STATE_S2:
    return STATE_S3;
  case STATE_S3:
    return STATE_S0;
  case STATE_S4:
    return STATE_S5;
  case STATE_S5:
    if(car.direction == DIR_WEST_L)
    {
      return STATE_S7;
    }
    else if(car.direction == DIR_WEST_S || car.direction == DIR_EAST_S)
    {
      return STATE_S6;
    }
    else
    {
      return STATE_S5;
    }
  case STATE_S6:
    return STATE_S0;
  case STATE_S7:
    return STATE_S2;
  default:
    return STATE_COUNT;
  }
}

static bool tryPassCarFromQueue(stateName_E state, carQueue_S *queue)
{

  if (isQueueEmpty(queue))
    return false;

  bool carCanPass = false;

  // Get current direction of the first car of the queue
  car_S car;

  if(front(queue, &car) == false)
  {
    return false;
  }

  // Check current state and set if the car can pass or not
  switch (state)
  {
  case STATE_S0:
    if(car.direction == DIR_EAST_S || car.direction == DIR_WEST_S || 
       car.direction == DIR_WEST_L)
    {
      carCanPass = true;
    }
    break;
    // Add more states here (only the ones that let cars pass)
  case STATE_S1:
    if(car.direction == DIR_WEST_S || car.direction == DIR_WEST_L)
    {
      carCanPass = true;
    }
  break;

  case STATE_S2:
    if(car.direction == DIR_WEST_L || car.direction == DIR_WEST_S ||
       car.direction == DIR_NORTH_R)
    {
      carCanPass = true;
    }
    break;

  case STATE_S3:
    if(car.direction == DIR_WEST_S ||car.direction == DIR_WEST_L)
    {
      carCanPass = true;
    }
  break;

  case STATE_S5:
    if(car.direction == DIR_NORTH_R || car.direction == DIR_NORTH_L)
    {
      carCanPass = true;
    }
  break;

  case STATE_S7:
    if(car.direction == DIR_NORTH_R)
    {
      carCanPass = true;
    }
  default:
    break;
  }

  
  if (carCanPass)
  {
    if(dequeue(queue, &car) == false)
    {
      return false;
    }
    return true;
  }

  return false;
}

//-------------------------------------------------------------------
// Main application code starts here
//-------------------------------------------------------------------
int main()
{
  srand(time(NULL));

  printf("Traffic Light Controller");

  // One-time initialzation of global values
  gState = STATE_S0;

  // Call SDK and hardware initialization functions
  stdio_init_all();
  gpioSetup();

  // Initialize our car queue
  carQueue_S carQueue;
  initQueue(&carQueue);

  // Main infinite loop logic
  while (true)
  {
    // Increment our tick
    tick_count += 1;

    // Add a new car to the queue each time button is pressed
    checkNewCarPressed(&carQueue);

    // Main processing every second
    if (tick_count - traffic_tick == 10)
    {
      traffic_tick = tick_count;

      // Get the next state depending on car in queue
      stateName_E nextState = getNextStateFromQueue(gState, &carQueue);

      // Update lights
      updateOutputs(gState);

      // Print current state and cars waiting on queue
      printf("State: S%i, ", gState);
      printQueue(&carQueue);

      // Try to get pass cars depending on lights state
      tryPassCarFromQueue(gState, &carQueue);

      // If nextState was STATE_COUNT, we stay in current state 
      if (nextState != STATE_COUNT)
      {
        gState = nextState;
      }
    }

    // Tick is 100ms
    sleep_ms(100);
  }
}
