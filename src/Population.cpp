#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <time.h>
#include "Fitness.h"
#include "CudaEval.h"

#define POPULATION_SIZE 150
#define POPULATION_PADDING 500
#define CHROMOSOME_LENGTH 903
#define CROSSES 1200000

typedef struct member_struct {
    char* chromosome;
    int num_cliques;
} MEMBER;

void PrintPopulation(MEMBER[]);
void SortInitialPopulation(MEMBER[], int, int);
void Cross(MEMBER*, MEMBER*, MEMBER*);
void InitializeRandomMember(MEMBER*);
void PrintMatrix(int[N][N]);
void InsertMember(MEMBER[], MEMBER);
void BiasedCross(MEMBER[2], MEMBER*);
void RandomSinglePointCross(MEMBER[2], MEMBER*);
void Mutate(MEMBER*);
int EvalAdj(char adj[N][N]);

int main(int argc, const char* argv[])
{
	CudaInit();

	while (1) {
		unsigned int seed = time(NULL);
		srand(seed); //init random seed

		MEMBER population[POPULATION_SIZE];

		for (int i = 0; i < POPULATION_SIZE; i++) {
			InitializeRandomMember(&population[i]);
		}

		SortInitialPopulation(population, 0, POPULATION_SIZE - 1);

		for (int i = 0; i < POPULATION_PADDING; i++) {
			MEMBER member;
			InitializeRandomMember(&member);
			InsertMember(population, member);
			//PrintPopulation(population);
		}

		/* breed children */
		int best = 999999;
		for (int i = 0; i < CROSSES; i++) {
			MEMBER parents[2];
			MEMBER child;

			parents[0] = population[rand() % POPULATION_SIZE];
			parents[1] = population[rand() % POPULATION_SIZE];

			BiasedCross(parents, &child);
			InsertMember(population, child);

			if (population[0].num_cliques < best) {
				best = population[0].num_cliques;
				std::cout << best << std::endl;
			}

			if (population[POPULATION_SIZE - 1].num_cliques == population[0].num_cliques) {
				for (int i = 5; i < POPULATION_SIZE; i++) {
					free(population[i].chromosome);
					InitializeRandomMember(&population[i]);
				}
			}
		}

		std::cout << "Best member: " << population[0].num_cliques << std::endl;

		std::ofstream outfile;

		outfile.open("results.txt", std::ios_base::app);

		outfile << std::endl;
		outfile << "Best member: " << population[0].num_cliques << std::endl;
		for (int j = 0; j < CHROMOSOME_LENGTH; j++) {
			outfile << (char) (population[0].chromosome[j] + 0x30);
		}

		outfile << std::endl;

		/* echo seed for posterity */
		outfile << "SEED: " << seed << std::endl;
		outfile.close();

		for (int i = 0; i < POPULATION_SIZE; i++) {
			free(population[i].chromosome);
		}
	}
}

/*
 * Prints an array of MEMBER.
 */
void PrintPopulation(MEMBER population[])
{
    for (int i = 0; i < POPULATION_SIZE; i++) {
        std::cout << "Member " << std::setw(3) << i + 1 << ": " ;
        std::cout << std::setw(4) << population[i].num_cliques << std::endl;
    }
    std::cout << std::endl;
}

/*
 * Performs a quicksort on an array of MEMBER.
 */
void SortInitialPopulation(MEMBER population[], int left, int right)
{
    int i = left;
    int j = right;
    int pivot = population[(left + right) / 2].num_cliques;
    MEMBER temp;

    /* partition */
    while (i <= j) {
        while (population[i].num_cliques < pivot) {
            i++;
        }
        while (population[j].num_cliques > pivot) {
            j--;
        }
        if (i <= j) {
            temp = population[i];
            population[i] = population[j];
            population[j] = temp;
            i++;
            j--;
        }
    };

    /* recursively sort either side of pivot */
    if (left < j) {
        SortInitialPopulation(population, left, j);
    }
    if (i < right) {
        SortInitialPopulation(population, i, right);
    }

    return;
}

void InsertMember(MEMBER population[], MEMBER member)
{
	if (member.num_cliques < population[POPULATION_SIZE - 1].num_cliques) {
		free(population[POPULATION_SIZE - 1].chromosome);
		population[POPULATION_SIZE - 1] = member;
		for (int i = POPULATION_SIZE - 1; i > 0; i--) {
			if (population[i].num_cliques < population[i - 1].num_cliques) {
				population[i] = population[i - 1];
				population[i - 1] = member;
			} else {
				break;
			}
		}
	} else {
		free(member.chromosome);
	}
}

void InitializeRandomMember(MEMBER *member)
{
    char *child_chromosome = (char*)(malloc(sizeof(char) * CHROMOSOME_LENGTH));
    for (int i = 0; i < CHROMOSOME_LENGTH; i++) {
        child_chromosome[i] = rand() % 2;
    }

	char adjacency_matrix[N][N];
    GetAdjacencyMatrixFromCharArray(child_chromosome, adjacency_matrix);
    int num_cliques = EvalAdj(adjacency_matrix);
    member->chromosome = child_chromosome;
    member->num_cliques = num_cliques;
}

void PrintMatrix(int arr[N][N]) {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			std::cout << arr[i][j] << " ";
		}
		std::cout << std::endl;
	}
}

void Mutate(MEMBER *member) {
	int bit = rand() % CHROMOSOME_LENGTH;
	member->chromosome[bit] ^= 1;
}

void BiasedCross(MEMBER parents[2], MEMBER *child)
{
	char *chromosome[2];
    chromosome[0] = parents[0].chromosome;
    chromosome[1] = parents[1].chromosome;

    char *child_chromosome = (char*)(malloc(sizeof(char) * CHROMOSOME_LENGTH));

	float bias;
	float parent_cliques[2];
	parent_cliques[0] = (float)parents[0].num_cliques;
	parent_cliques[1] = (float)parents[1].num_cliques;

	bias = parent_cliques[parent_cliques[0] < parent_cliques[1]] / (parent_cliques[0] + parent_cliques[1]);
	MEMBER bad;
	MEMBER good;
	if (parent_cliques[0] < parent_cliques[1]) {
		bad = parents[1];
		good = parents[0];
	} else {
		bad = parents[0];
		good = parents[1];
	}

	for (int i = 0; i < CHROMOSOME_LENGTH; i++) {
		if (((float)rand() / (float)RAND_MAX) > bias) {
			child_chromosome[i] = bad.chromosome[i];
		} else {
			child_chromosome[i] = good.chromosome[i];
		}
	}

	char adjacency_matrix[N][N];
    GetAdjacencyMatrixFromCharArray(child_chromosome, adjacency_matrix);
    int num_cliques = EvalAdj(adjacency_matrix);

    child->chromosome = child_chromosome;
    child->num_cliques = num_cliques;
}

void RandomSinglePointCross(MEMBER parents[2], MEMBER *child)
{
	char *chromosome[2];
    chromosome[0] = parents[0].chromosome;
    chromosome[1] = parents[1].chromosome;

    char *child_chromosome = (char*)(malloc(sizeof(char) * CHROMOSOME_LENGTH));

    int crossover = rand() % CHROMOSOME_LENGTH;
    
	for (int i = 0; i < crossover; i++) {
        child_chromosome[i] = chromosome[0][i];
    }

    for (int i = crossover; i < CHROMOSOME_LENGTH; i++) {
        child_chromosome[i] = chromosome[1][i];
    }

	char adjacency_matrix[N][N];
    GetAdjacencyMatrixFromCharArray(child_chromosome, adjacency_matrix);
    int num_cliques = EvalAdj(adjacency_matrix);

    child->chromosome = child_chromosome;
    child->num_cliques = num_cliques;
}

int EvalAdj(char adj[N][N]) {
	return CudaEval((char *) adj);
}