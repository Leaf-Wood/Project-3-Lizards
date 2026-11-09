/***************************************************************/
/*                                                             */
/* lizard.cpp                                                  */
/*                                                             */
/* To compile, you need all the files listed below             */
/*   lizard.cpp                                                */
/*                                                             */
/* Be sure to use the -lpthread option for the compile command */
/*   g++ -g -Wall -std=c++11 lizard.cpp -o lizard -pthread     */
/*                                                             */
/* Execute with the -d command-line option to enable debugging */
/* output.  For example,                                       */
/*   ./lizard -d                                               */
/*                                                             */
/***************************************************************/

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <semaphore.h> // NG
#include <cerrno> // NG


using namespace std;

/*
 * This is a stub file.  It contains very little code and what
 * it does contain may need to be altered or removed.  It is
 * only provided for a starting point.
 *
 * The comments are probably useful.
 */


/*
 * Define "constant" values here
 */

/*
 * Make this 1 to check for lizards travelling in both directions
 * Leave it 0 to allow bidirectional travel
 */
#define UNIDIRECTIONAL       1 // NG: Share the driveway in one direction at a time.

/*
 * Set this to the number of seconds you want the lizard world to
 * be simulated.  
 * Try 30 for development and 120 for more thorough testing.
 */
#define WORLDEND             30
// NG: Keep the 30-second setting for quick tests. Give the submitted run 180 seconds.
#undef WORLDEND
#define WORLDEND 180

/*
 * Number of lizard threads to create
 */
#define NUM_LIZARDS          20

/*
 * Number of cat threads to create
 */
#define NUM_CATS             2

/*	
 * Maximum lizards crossing at once before alerting cats
 */
#define MAX_LIZARD_CROSSING  4

/*
 * Maximum seconds for a lizard to sleep
 */
#define MAX_LIZARD_SLEEP     3

/*
 * Maximum seconds for a cat to sleep
 */
#define MAX_CAT_SLEEP        3

/*
 * Maximum seconds for a lizard to eat
 */
#define MAX_LIZARD_EAT       5

/*
 * Number of seconds it takes to cross the driveway
 */
#define CROSS_SECONDS        2


/*
 * Declare global variables here
 */


/**************************************************/
/* Please leave these variables alone.  They are  */
/* used to check the proper functioning of your   */
/* program.  They should only be used in the code */
/* I have provided.                               */
/**************************************************/
int numCrossingSago2MonkeyGrass;
int numCrossingMonkeyGrass2Sago;
int debug;
int running;
/**************************************************/
// NG: One ticket per open driveway spot. Both directions share the pool.
sem_t drivewaySlots; // NG
mutex crossingMutex; // NG
mutex runningMutex; // NG: Protect the flag that tells the cats and lizards to stop.

// NG: Keep track of lizards admitted in the current direction.
enum class CrossingDirection { ToMonkeyGrass, ToSago };
static mutex admissionMutex;
static sem_t emptyDriveway; // One permit when no lizards have reserved a crossing.
static CrossingDirection activeDirection = CrossingDirection::ToMonkeyGrass;
static int reservedCrossings = 0; // Includes a lizard still waiting for its driveway ticket.

static bool worldRunning()
{
    runningMutex.lock();
    const bool stillRunning = running != 0;
    runningMutex.unlock();
    return stillRunning;
} // NG


// NG: Keep waiting for a ticket if a signal interrupts the wait.
static void waitForPermit(sem_t& permits)
{
    int result;
    do {
        result = sem_wait(&permits);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        perror("sem_wait");
        exit(1);
    }
}

static void takeDrivewaySlot(CrossingDirection direction)
{
    if (!UNIDIRECTIONAL) {
        waitForPermit(drivewaySlots);
        return;
    }

    // One lizard waits at the entrance. Later arrivals wait behind it.
    lock_guard<mutex> nextInLine(admissionMutex);
    crossingMutex.lock();
    if (reservedCrossings == 0 || activeDirection != direction) {
        // Let the lizards already admitted finish and clear the driveway.
        crossingMutex.unlock();
        waitForPermit(emptyDriveway);
        crossingMutex.lock();
        activeDirection = direction;
    }
    ++reservedCrossings;
    crossingMutex.unlock();

    waitForPermit(drivewaySlots);
}

static void returnDrivewaySlot()
{
    sem_post(&drivewaySlots);
    if (UNIDIRECTIONAL) {
        lock_guard<mutex> lock(crossingMutex);
        if (--reservedCrossings == 0) {
            sem_post(&emptyDriveway); // Last lizard out lets the next direction in.
        }
    }
} // NG



/**
 * This class models a cat that sleep, wakes-up, checks on lizards in the driveway
 * and goes back to sleep. If the cat sees enough lizards it "plays" with them.
 */
class Cat {
	
	int       _id;        // the Id of the cat
	thread*   _catThread; // the thread simulating the cat
	
	public:
		Cat (int id);
        ~Cat() { delete _catThread; } // NG: Free the cat thread after it finishes.
		int getId();
		void run();
		void wait();
    
    private:
		void sleepNow();

        static void catThread (Cat *aCat); 
};

/**
 * Constructs a cat.
 *
 * @param id - the Id of the cat 
 */
Cat::Cat (int id)
{
	_id = id;
    _catThread = nullptr; // NG
}

/**
 * Returns the Id of the cat.
 *
 * @return the Id of a cat
 */
int Cat::getId()
{
	return _id;
}


/**
 * Launches a cat thread.
 * 
 * Status: Incomplete - Make changes to this code.
 */
 void Cat::run() 
 {
	 // launch the thread to simulate the cat's behavior	 
    _catThread = new thread(catThread, this); // NG
	 
 }
 
 /**
  * Waits for a cat to finish running.
  *
  * Status: Incomplete - Make changes to this code.
  */
 void Cat::wait()
 {
	 // wait for the thread to terminate
    if (_catThread != nullptr) _catThread->join(); // NG
 }
 
 
/**
 * Simulate a cat sleeping for a random amount of time
 *
 *
 * @return N/A
 *
 * Status: Complete - No need to change any of this code.
 */
void Cat::sleepNow()
{
	int sleepSeconds;

	sleepSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_CAT_SLEEP);

	if (debug)
    {
		cout << "[" << _id << "] cat sleeping for " << sleepSeconds << " seconds" << endl;
		cout << flush;
    }

	sleep( sleepSeconds );

	if (debug)
    {
		cout << "[" << _id << "] cat awake" << endl;
		cout << flush;
    }
}

/**
  *
  * This simulates a cat that is sleeping and occasionally checking on
  * the driveway on lizards.
  * 
  * @param aCat - a cat that is being run concurrently
  * 
  * @return - N/A
  *
  * Status: Incomplete - Make changes as you see are necessary.
  */
void Cat::catThread (Cat *aCat)
{
	
	if (debug)
    {
		cout << "[" << aCat->getId() << "] cat is alive\n";
		cout << flush;
    }

	while (worldRunning())
    {
		aCat->sleepNow();



		/*
	     * Check for too many lizards crossing
	     */
        lock_guard<mutex> lock(crossingMutex); // NG: One cat peek sees both crossing counts together.
		if (numCrossingSago2MonkeyGrass + numCrossingMonkeyGrass2Sago > MAX_LIZARD_CROSSING)
		{
		  cout << "\tThe cats are happy - they have toys.\n";
		  exit( -1 );
		}
    }
}

 

class Lizard {
	int     _id;      // the Id of the lizard
	thread* _aLizard; // the thread simulating the lizard

	public:
		Lizard(int id);
        ~Lizard() { delete _aLizard; } // NG: Free the lizard thread after it finishes.
		int getId();
        void run();
        void wait();

    private:
		void sago2MonkeyGrassIsSafe();
		void crossSago2MonkeyGrass();
		void madeIt2MonkeyGrass();
		void eat();
		void monkeyGrass2SagoIsSafe();
		void crossMonkeyGrass2Sago();
		void madeIt2Sago();
		void sleepNow();

        static void lizardThread(Lizard *aLizard);
	
};


/**
 * Constructs a lizard.
 *
 * @param id - the Id of the lizard 
 */
Lizard::Lizard (int id)
{
	_id = id;
}

/**
 * Returns the Id of the lizard.
 *
 * @return the Id of a lizard
 * 
 * Status: Complete - No need to change any of this code
 */
int Lizard::getId()
{
	return _id;
}

/**
 * Launches a lizard thread.
 * 
 * Status: Complete - No need to change any of this code.
 */
 void Lizard::run() 
 {
	 // launch the thread to simulate the lizard's behavior
    _aLizard = new thread ( lizardThread, this); 
 }
 
 /**
  * Waits for a lizard to finish running.
  *
  * Status: Complete - No need to change any of this code.
  */
 void Lizard::wait()
 {
	 // wait for the thread to terminate
	if (_aLizard != NULL) {
        _aLizard->join();
    } 
 }
 

/**
 * Simulate a lizard sleeping for a random amount of time
 *
 * Status: Complete - No need to change any of this code.
 */
void Lizard::sleepNow()
{
	int sleepSeconds;

	sleepSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_LIZARD_SLEEP);

	if (debug)
    {
      cout << "[" << _id << "] sleeping for " << sleepSeconds << " seconds" << endl;
      cout << flush;
    }

	sleep( sleepSeconds );

	if (debug)
    {
      cout << "[" << _id << "] awake" << endl;
      cout << flush;
    }
}



 
/**
 *
 * Returns when it is safe for this lizard to cross from the sago
 * to the monkey grass.   Should use some synchronization 
 * facilities (lock/semaphore) here.
 *
 * Status: Incomplete - Make changes as you see are necessary.
 */
void Lizard::sago2MonkeyGrassIsSafe()
{
	if (debug)
    {
		cout << "[" << _id << "] checking  sago -> monkey grass" << endl;
		cout << flush;
    }
    takeDrivewaySlot(CrossingDirection::ToMonkeyGrass); // NG

	


	if (debug)
    {
		cout << "[" << _id << "] thinks  sago -> monkey grass  is safe" << endl;
		cout << flush;
    }
}


/**
 *
 * Delays for 1 second to simulate crossing from the sago to
 * the monkey grass. 
 *
 * Status: Incomplete - Make changes as you see are necessary.
 */
void Lizard::crossSago2MonkeyGrass()
{
	if (debug)
    {
      cout << "[" << _id << "] crossing  sago -> monkey grass" << endl;
      cout << flush;
    }

	/*
	 * One more crossing this way
	 */
    { // NG
        lock_guard<mutex> lock(crossingMutex); // NG
	numCrossingSago2MonkeyGrass++;
    if (debug) printf("Lizards crossing: %d\n", numCrossingSago2MonkeyGrass + numCrossingMonkeyGrass2Sago); // NG

	/*
     * Check for lizards cross both ways
     */
	if (numCrossingMonkeyGrass2Sago && UNIDIRECTIONAL)
    {
		cout << "\tCrash!  We have a pile-up on the concrete." << endl;
		cout << "\t" << numCrossingSago2MonkeyGrass << " crossing sago -> monkey grass" << endl;
		cout << "\t" << numCrossingMonkeyGrass2Sago << " crossing monkey grass -> sago" << endl;
		exit( -1 );
    }
    } // NG: Unlock while the lizard crosses. It keeps its ticket.


	/*
     * It takes a while to cross, so simulate it
     */
	sleep( CROSS_SECONDS );

    /*
     * That one seems to have made it
     */
    { // NG
        lock_guard<mutex> lock(crossingMutex); // NG
    numCrossingSago2MonkeyGrass--;
    } // NG
}


/**
 * Tells others they can go now
 *
 * Status: Incomplete - Make changes as you see are necessary.
 */
void Lizard::madeIt2MonkeyGrass()
{
	/*
     * Whew, made it across
     */
	if (debug)
    {
		cout << "[" << _id << "] made the  sago -> monkey grass  crossing" << endl;
		cout << flush;
    }
    returnDrivewaySlot(); // NG: Return the ticket so another lizard can cross.





}


/**
 * Simulate a lizard eating for a random amount of time
 *
 * Status: Complete - No need to change any of this code.
 */
void Lizard::eat()
{
	int eatSeconds;

	eatSeconds = 1 + (int)(random() / (double)RAND_MAX * MAX_LIZARD_EAT);

	if (debug)
    {
		cout << "[" << _id << "] eating for " << eatSeconds << " seconds" << endl;
		cout << flush;
    }

	/*
     * Simulate eating by blocking for a few seconds
     */
	sleep( eatSeconds );

	if (debug)
    {
      cout << "[" << _id << "] finished eating" << endl;
      cout << flush;
    }
}


/**
 * Returns when it is safe for this lizard to cross from the monkey
 * grass to the sago.   Should use some synchronization 
 * facilities (lock/semaphore) here.
 *
 * Status: Incomplete - Make changes as you see are necessary.
 */
void Lizard::monkeyGrass2SagoIsSafe()
{
	if (debug)
    {
		cout << "[" << _id << "] checking  monkey grass -> sago" << endl;
		cout << flush;
    }




    takeDrivewaySlot(CrossingDirection::ToSago); // NG

	if (debug)
    {
		cout << "[" << _id << "] thinks  monkey grass -> sago  is safe" << endl;
		cout << flush;
    }
}



/**
 * Delays for 1 second to simulate crossing from the monkey
 * grass to the sago. 
 *
 * Status: Incomplete - Make changes as you see are necessary.
 */
void Lizard::crossMonkeyGrass2Sago()
{
	if (debug)
    {
		cout << "[" << _id << "] crossing  monkey grass -> sago" << endl;
		cout << flush;
    }

    /*
     * One more crossing this way
     */
    { // NG
        lock_guard<mutex> lock(crossingMutex); // NG
	numCrossingMonkeyGrass2Sago++;
    if (debug) printf("Lizards crossing: %d\n", numCrossingSago2MonkeyGrass + numCrossingMonkeyGrass2Sago); // NG

  
    /*
     * Check for lizards cross both ways
     */
	if (numCrossingSago2MonkeyGrass && UNIDIRECTIONAL)
    {
		cout << "\tOh No!, the lizards have cats all over them." << endl;
		cout << "\t " << numCrossingSago2MonkeyGrass << " crossing sago -> monkey grass" << endl;
		cout << "\t " << numCrossingMonkeyGrass2Sago << " crossing monkey grass -> sago" << endl;
		exit( -1 );
    }
    } // NG: Unlock while the lizard crosses. It keeps its ticket.

	/*
     * It takes a while to cross, so simulate it
     */
	sleep( CROSS_SECONDS );

	/*
     * That one seems to have made it
     */
    { // NG
        lock_guard<mutex> lock(crossingMutex); // NG
	numCrossingMonkeyGrass2Sago--;
    } // NG
}


/**
 *
 * Tells others they can go now
 *
 * Status: Incomplete - Make changes as you see are necessary.
 */
void Lizard::madeIt2Sago()
{
	/*
     * Whew, made it across
     */
	if (debug)
    {
		cout << "[" << _id << "] made the  monkey grass -> sago  crossing" << endl;
		cout << flush;
    }
    returnDrivewaySlot(); // NG: Return the ticket so another lizard can cross.
}

/**
  * Follows the algorithm provided in the assignment
  * description to simulate lizards crossing back and forth
  * between a sago palm and some monkey grass. 
  *  
  * @param aLizard - the lizard to be executed concurrently
  *
  * @return N/A
  
  * Status: Incomplete - Make changes as you see are necessary.
  */
void Lizard::lizardThread(Lizard *aLizard)
{	
	if (debug)
    {
      cout << "[" << aLizard->getId() << "] lizard is alive" << endl;
      cout << flush;
    }

	while (worldRunning())
    {
      /* 
       * Follow the algorithm given in the assignment
       * using calls to the functions declared above.
       * You'll need to complete the implementation of
       * some functions by filling in the code.  Some  
       * are already completed - see the comments.
       */
        // NG: One lizard round trip. Nap, cross for a snack, then head home.
        aLizard->sleepNow();
        if (!worldRunning()) break; // Time up? Skip the trip.

        aLizard->sago2MonkeyGrassIsSafe(); // Wait for a ticket to the monkey grass.
        if (!worldRunning()) {
            returnDrivewaySlot(); // Time up? Return the ticket and stop.
            break;
        }
        aLizard->crossSago2MonkeyGrass();
        aLizard->madeIt2MonkeyGrass(); // Made it to the monkey grass. Return the ticket.

        if (!worldRunning()) break; // Time up? Skip the snack.
        aLizard->eat();
        if (!worldRunning()) break; // Time up? Skip the trip home.

        aLizard->monkeyGrass2SagoIsSafe(); // Wait for a ticket to the sago palm.
        if (!worldRunning()) {
            returnDrivewaySlot(); // Time up? Return the ticket and stop.
            break;
        }
        aLizard->crossMonkeyGrass2Sago();
        aLizard->madeIt2Sago(); // Made it to the sago palm. Return the ticket. // NG


















    }

}
 



/*
 * main()
 *
 * Should initialize variables, locks, semaphores, etc.
 * Should start the cat thread and the lizard threads
 * Should block until all threads have terminated
 * Status: Incomplete - Make changes to this code.
 */
int main(int argc, char **argv)
{
	/*
	 * Declare local variables
     */




	/*
     * Check for the debugging flag (-d)
     */
	debug = 0;
	if (argc > 1)
		if (strncmp(argv[1], "-d", 2) == 0)
			debug = 1;


	/*
     * Initialize variables
     */
	numCrossingSago2MonkeyGrass = 0;
	numCrossingMonkeyGrass2Sago = 0;
	running = 1;


	/*
     * Initialize random number generator
	 */
	srandom( (unsigned int)time(NULL) );


	/*
     * Initialize locks and/or semaphores
     */
    sem_init(&drivewaySlots, 0, MAX_LIZARD_CROSSING); // NG: Start with one ticket per spot.
    if (UNIDIRECTIONAL) sem_init(&emptyDriveway, 0, 1); // NG: The driveway starts empty.




	/*
     * Create NUM_LIZARDS lizard threads
     */
    vector<Lizard*> allLizards;
    for (int i=0; i < NUM_LIZARDS; i++) {
	    allLizards.push_back(new Lizard(i));
    }    
    

    /*
     * Create NUM_CATS cat threads
     */
	 
    vector<Cat*> allCats; // NG
    for (int i = 0; i < NUM_CATS; i++) { // NG
        allCats.push_back(new Cat(i)); // NG
    } // NG

	/*
	 * Run NUM_LIZARDS and NUM_CATS threads
	 */
    for (int i=0; i < NUM_LIZARDS; i++) {
        allLizards[i]->run();
    }
    for (Cat* cat : allCats) cat->run(); // NG

	/*
     * Now let the world run for a while
     */
	sleep( WORLDEND );


	/*
     * That's it - the end of the world
     */
    // NG: Tell the cats and lizards to call it a day. Unlock before joining.
    runningMutex.lock();
    running = 0;
    runningMutex.unlock();
    // NG: Lizards return tickets after crossing or if time is up when they get one.


    /*
     * Wait until all threads terminate
     */
    for (Lizard* lizard : allLizards) lizard->wait(); // NG
    for (Cat* cat : allCats) cat->wait(); // NG






	/*
     * Delete the locks and semaphores
     */
    sem_destroy(&drivewaySlots); // NG
    if (UNIDIRECTIONAL) sem_destroy(&emptyDriveway); // NG
	 
	 
	 
	/*
	 * Delete all cat and lizard objects
	 */
    for (Lizard* lizard : allLizards) delete lizard; // NG
    for (Cat* cat : allCats) delete cat; // NG
 



	/*
     * Exit happily
     */
	return 0;
}






