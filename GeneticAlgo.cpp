// ConsoleApplication11.cpp : Defines the entry point for the console application.
//
/*--------------------------------INCLUDES------------------------------------*/
#include "stdafx.h"
#include "Vector"
#include "Set"
#include <algorithm>    // std::sort
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>


// Types
/*--------------------------------TYPES------------------------------------*/
typedef std::vector<int> t_chessboard;

typedef struct
{
	t_chessboard member;
	int fitnessValue;
	double accumulatedSurvival;
	double survival;
} t_populationMember;
typedef std::vector<t_populationMember> t_population;

typedef struct
{
	unsigned groupfitness;
	unsigned bestFitness;
	bool winner_found;
} t_results;

/*--------------------------------GLOBALS------------------------------------*/
static unsigned bestScoreThroughoutRuntime = 0;
static t_chessboard winningBoard;

/*--------------------------PRIVATE FUNCTIONS---------------------------------*/


/*-FUNCT: SELECTION-------------------------------------*/
static unsigned weightedSelection(t_population &population)
{
	// Random # between 0 and 1
	double random_integer = (double) rand() / RAND_MAX;

	// Edge case
	if (random_integer > (double)population[population.size() - 1].accumulatedSurvival)
		return population.size() - 1;

	// Find the index that satisfies this randomly selected number
	for (int i = 0; i < population.size(); i++)
	{
		if (population[i].accumulatedSurvival >= random_integer)
			return i;
	}

	// Should never occur
	return 0xFFFFFFFF;
}
/*-FUNCT: FITNESS-------------------------------------*/
static void calculateFitness(t_populationMember &popMember)
{
	// Variables we'll need to calculate fitness
	unsigned n = popMember.member.size();
	unsigned maxCollisions = (n * (n - 1)) / 2;

	// Calculate direct clashes as a function of unique entries
	std::set<unsigned> x(popMember.member.begin(), popMember.member.end());
	unsigned uniqueEntries = popMember.member.size() - x.size();

	// Add the diagonal clashes as well

	unsigned diagonalClashes = 0;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			if (i != j)
			{
				unsigned deltaX = abs(i - j);
				unsigned deltay = abs(popMember.member[i] - popMember.member[j]);
				if (deltaX == deltay)
					diagonalClashes++;

			}
		}
	}
	// Where X1 and X2 are diagnoally clashing, we donly want to count that once.
	diagonalClashes = diagonalClashes / 2;

	// Fitness value must be maximized
	popMember.fitnessValue = maxCollisions - (diagonalClashes + uniqueEntries);

	// We messed up somewhere...
	if (popMember.fitnessValue < 0)
	{
		printf("ERROR!!!\r\n");
		while (1);
	}
	return;

}
/*-FUNCT: PROCESS POPULATION--------------------------------*/
static void processPopulation(t_population &population, t_results &results)
{

	unsigned populationSize = population.size();
	unsigned n_size = population[0].member.size();

	// We've now created a random initial population
	// Now, for each, calculate the fitness function
	for (int i = 0; i < populationSize; i++)
		calculateFitness(population[i]);

	// Calculate group fitness
	unsigned populationFitness = 0;
	for (int i = 0; i < populationSize; i++)
		populationFitness += population[i].fitnessValue;

	// Now, for each, calculate the survival chances
	for (int i = 0; i < populationSize; i++)
		population[i].survival =
		(double)population[i].fitnessValue / populationFitness;

	// Accumulate probability of survival for ability to perform selection
	population[0].accumulatedSurvival = population[0].survival;
	for (int i = 1; i < populationSize; i++)
		population[i].accumulatedSurvival =
		population[i].survival + population[i - 1].accumulatedSurvival;

	results.winner_found = false;
	results.bestFitness = population[0].fitnessValue;
	// Look for a winner
	for (int i = 0; i < populationSize; i++)
	{
		if (population[i].fitnessValue ==
			(n_size * (n_size - 1)) / 2)
		{
			results.winner_found = true;
			winningBoard = population[i].member;
		}

		if (population[i].fitnessValue > results.bestFitness)
			results.bestFitness = population[i].fitnessValue;
	}

	results.groupfitness = populationFitness;

	return;
}
/*-FUNCT: PERFORM SELECTION--------------------------------*/
static void performSelection(t_population &population)
{
	unsigned populationSize = population.size();
	// Select from this population
	t_population temp(population.begin(), population.end());
	for (int i = 0; i < populationSize; i++)
	{
		t_populationMember selectedMember;
		unsigned selectedIdx = weightedSelection(temp);

		selectedMember = population[selectedIdx];
		population[i] = selectedMember;
	}
}
/*-FUNCT: PERFORM CROSSOVER--------------------------------*/
static void performCrossover(t_population &population)
{
	unsigned populationSize = population.size();
	// Select from this population
	for (int i = 1; i < populationSize; i++)
	{
		// Get a random point to join the chromosomes
		unsigned crossOverPoint = rand() % population[i].member.size();
		//crossOverPoint = population.size() / 2;
		t_populationMember child1, child2;
		for (int j = 0; j < population[i].member.size(); j++)
		{

			if (j < crossOverPoint)
			{
				child1.member.push_back(population[i - 1].member[j]);
				child2.member.push_back(population[i].member[j]);
			}
			else
			{
				child1.member.push_back(population[i].member[j]);
				child2.member.push_back(population[i - 1].member[j]);
			}

		}
		population[i - 1] = child2;
		population[i] = child1;
	}
}
/*-FUNCT: PERFORM MUTATION--------------------------------*/
static void performMutation(t_population &population)
{
	// the higher the less mutation (1 - unsignedmax)
	const unsigned MUTATION_RATE = 51; 
	unsigned populationSize = population.size();
	unsigned n = population[0].member.size();

	for (int i = 0; i < populationSize; i++)
	{
		// Get a random idx to change (could be no idx at all)
		unsigned randomIdxFlip = rand() % (n * MUTATION_RATE);
		// Get a random value to change it to
		unsigned randomValue = rand() % n;
		// Could result in no change
		if (randomIdxFlip < n)
			population[i].member[randomIdxFlip] = randomValue;
	}
}

/*-FUNCT: MAIN PROGRAM----------------------------------*/

static int goNQueens(unsigned n)
{
	// Max possible collisions
	unsigned maxCollisions = (n * (n - 1)) / 2;
	unsigned retValNumIter = 0xFFFFFFFF;
	const unsigned POPULATION_SIZE = 10000; // Random value

										 // Init each chessboard
	t_population population;

	for (int i = 0; i < POPULATION_SIZE; i++)
	{
		t_populationMember popMem;
		for (int j = 0; j < n; j++)
		{
			// Generate a random position for this column
			unsigned random_integer = rand() % n;
			popMem.member.push_back(random_integer);
		}

		population.push_back(popMem);

	}

	t_results results;
	processPopulation(population, results);
	printf("The initial group fitness was %u\r\n", results.groupfitness);
	std::ofstream file;
	std::remove("D:\\classLog.txt");	
	std::remove("D:\\winner.txt");

	while (results.winner_found != true)
	{

		performSelection(population);

		performCrossover(population);

		performMutation(population);

		// Calculate things like fitness, survival, selection probability...
		processPopulation(population, results);
		bestScoreThroughoutRuntime =
			(results.bestFitness > bestScoreThroughoutRuntime ?
				results.bestFitness : bestScoreThroughoutRuntime);

		printf("Group Fitness: %u | Best Score: %u | All Time Best: %u\r\n",
			results.groupfitness, results.bestFitness,
			bestScoreThroughoutRuntime);
		
		file.open("D:\\classLog.txt", std::ios_base::app);
		file << results.groupfitness << "," << results.bestFitness << "," << bestScoreThroughoutRuntime << std::endl;
		file.close();
	}
	printf("Done!\r\n");
	printf("The winning board was:\n");

	file.open("D:\\winner.txt", std::ios_base::app);
	for (int i = 0; i < winningBoard.size(); i++)
	{
		printf("%u ", winningBoard[i]);
		file << winningBoard[i] << " ";
	}
	file << std::endl;
	file.close();
	printf("\r\n");
    
	return retValNumIter;

}
/*-FUNCT: ENTRY PT-----------------------------------*/
int _tmain(int argc, _TCHAR* argv[])
{
	unsigned numIterations;
	srand((unsigned)time(0));

	numIterations = goNQueens(32);
	return 0;
}


