#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>

/* 
 * This simple default synchronization mechanism allows only creature at a time to
 * eat.   The globalCatMouseSem is used as a a lock.   We use a semaphore
 * rather than a lock so that this code will work even before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */

/* bowlsArray will store each bowl, with a value of 1 indicating that a creature is eating
 * from that bowl, 0 if no creature is eating from that bowl.
 */

static unsigned int *bowlsArray;

static struct lock *lk_mutex;
static volatile int numCatsEating = 0;
static volatile int numMiceEating = 0;
static struct cv *cv_canEat;

/* 
 * The CatMouse simulation will call this function once before any cat or
 * mouse tries to each.
 *
 * You can use it to initialize synchronization and other variables.
 * 
 * parameters: the number of bowls
 */
void
catmouse_sync_init(int bowls)
{
  /* replace this default implementation with your own implementation of catmouse_sync_init */

    //First member of array will always be empty, and we treat the second
    //element as the first one in order to correspond with the bowl numbers which start from
    //1 instead of 0.

    bowlsArray = (unsigned int *)kmalloc(sizeof(unsigned int)*(bowls+1));

    //All bowls begin with no creatures eating from them
    for(int i=0; i < bowls + 1; i++) {
      bowlsArray[i] = 0;
    }

    lk_mutex = lock_create("lk_mutex");

    if (lk_mutex == NULL) {
    panic("could not create global bowl_mutex synchronization semaphore");
    }

    cv_canEat= cv_create("cv_canEat");

    if (cv_canEat == NULL) {
    panic("could not create global cv_canEat synchronization semaphore");
    }

  //(void)bowls; /* keep the compiler from complaining about unused parameters */

  return;
}

/* 
 * The CatMouse simulation will call this function once after all cat
 * and mouse simulations are finished.
 *
 * You can use it to clean up any synchronization and other variables.
 *
 * parameters: the number of bowls
 */
void
catmouse_sync_cleanup(int bowls)
{
  /* replace this default implementation with your own implementation of catmouse_sync_cleanup */

  KASSERT(lk_mutex != NULL);
  lock_destroy(lk_mutex);

  KASSERT(cv_canEat != NULL);
  cv_destroy(cv_canEat);

  KASSERT(bowlsArray != NULL);
  kfree(bowlsArray);
  bowlsArray = NULL;

  (void)bowls; /* keep the compiler from complaining about unused parameters */
}


/*
 * The CatMouse simulation will call this function each time a cat wants
 * to eat, before it eats.
 * This function should cause the calling thread (a cat simulation thread)
 * to block until it is OK for a cat to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the cat is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_before_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of cat_before_eating */

  lock_acquire(lk_mutex);

  while(numMiceEating != 0 || bowlsArray[bowl] != 0){

    cv_wait(cv_canEat, lk_mutex);

  };

  numCatsEating++;

  //A cat will now be eating from this bowl
  bowlsArray[bowl] = 1; 

  lock_release(lk_mutex);

  //(void)bowl;  /* keep the compiler from complaining about an unused parameter */
}

/*
 * The CatMouse simulation will call this function each time a cat finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this cat finished.
 *
 * parameter: the number of the bowl at which the cat is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_after_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of cat_after_eating */

  lock_acquire(lk_mutex);

  numCatsEating--;

  //A cat is done eating from this bowl
  bowlsArray[bowl] = 0; 

  cv_broadcast(cv_canEat, lk_mutex);

  lock_release(lk_mutex);

  //(void)bowl;  /* keep the compiler from complaining about an unused parameter */
}

/*
 * The CatMouse simulation will call this function each time a mouse wants
 * to eat, before it eats.
 * This function should cause the calling thread (a mouse simulation thread)
 * to block until it is OK for a mouse to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the mouse is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_before_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of mouse_before_eating */

  lock_acquire(lk_mutex);

  while(numCatsEating != 0 || bowlsArray[bowl] != 0){

    cv_wait(cv_canEat, lk_mutex);

  };

  numMiceEating++;

  //A mouse will now be eating from this bowl
  bowlsArray[bowl] = 1;

  lock_release(lk_mutex);

  //(void)bowl;  /* keep the compiler from complaining about an unused parameter */
}

/*
 * The CatMouse simulation will call this function each time a mouse finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this mouse finished.
 *
 * parameter: the number of the bowl at which the mouse is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_after_eating(unsigned int bowl) 
{
  /* replace this default implementation with your own implementation of mouse_after_eating */

  lock_acquire(lk_mutex);

  numMiceEating--;

  //A mouse is done eating from this bowl
  bowlsArray[bowl] = 0; 

  cv_broadcast(cv_canEat, lk_mutex);

  lock_release(lk_mutex);

  //(void)bowl;  /* keep the compiler from complaining about an unused parameter */
}
