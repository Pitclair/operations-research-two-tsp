#include <math.h>
#include "tsp_solution.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "math_util.h"
#include "constants.h"
#include "enums.h"
#include "c_util.h"
#include "plot_util.h"
#include "util/chrono.h"

struct TspSolution
{
    double cost;
    unsigned long* tour;
    const TspInstance* const instance;
};

double calculate_solution_cost(const TspSolution* solution)
{
    return calculate_tour_cost(
        solution->tour,
        get_number_of_nodes(solution->instance),
        get_edge_cost_array(solution->instance)
    );
}


TspSolution* init_solution(const TspInstance* instance)
{
    const unsigned long number_of_nodes = get_number_of_nodes(instance);

    unsigned long* tour = calloc(tour_array_size(number_of_nodes), sizeof(number_of_nodes));
    check_alloc(tour);
    for (unsigned long i = 0; i < number_of_nodes; i++)
        tour[i] = i;
    tour[number_of_nodes] = tour[0];

    TspSolution* solution_ptr = malloc(sizeof(TspSolution));
    check_alloc(solution_ptr);
    const TspSolution solution = {
        .cost = calculate_tour_cost(tour, number_of_nodes, get_edge_cost_array(instance)),
        .tour = tour,
        .instance = instance
    };
    memcpy(solution_ptr, &solution, sizeof(solution));

    return solution_ptr;
}


FeasibilityResult check_solution_feasibility(const TspSolution* solution)
{
    const unsigned long number_of_nodes = get_number_of_nodes(solution->instance);
    unsigned long counter[number_of_nodes];
    memset(counter, 0, number_of_nodes * sizeof(counter[0]));

    for (unsigned long i = 0; i < number_of_nodes; i++)
    {
        // Checks that each value in the solution array is a valid index in the TSP problem's node array
        if (solution->tour[i] < 0 || solution->tour[i] > number_of_nodes - 1)
        {
            return UNINITIALIZED_ENTRY;
        }

        counter[solution->tour[i]]++;

        // Ensures each index appears exactly once; if an index appears more than once or not at all, the TSP solution is infeasible
        // This is because in a valid TSP tour, each node must be visited exactly once to form a Hamiltonian cycle
        if (counter[solution->tour[i]] != 1)
        {
            return DUPLICATED_ENTRY;
        }
    }

    // Recalculate the total cost of the current solution
    const double calculated_cost = calculate_solution_cost(solution);

    // Check if the stored cost is approximately equal to the recalculated cost within an epsilon margin
    // This accounts for floating-point precision errors
    if (fabs(solution->cost - calculated_cost) > EPSILON)
    {
        return NON_MATCHING_COST;
    }

    return FEASIBLE;
}


FeasibilityResult solve_tsp(const TspSolver solver, TspSolution* solution)
{
    solve_instance(solver,
                   rand() % get_number_of_nodes(solution->instance),
                   solution->tour,
                   &solution->cost,
                   solution->instance);
    return check_solution_feasibility(solution);
}

unsigned long tour_array_size(const unsigned long number_of_nodes)
{
    return number_of_nodes + 1;
}

void plot_solution(const TspSolution* sol, const char* output_name)
{
    plot_tour(sol->tour, get_number_of_nodes(sol->instance), get_nodes(sol->instance), output_name);
}

FeasibilityResult solve_tsp_for_seconds(const TspSolver solver, TspSolution* solution, const unsigned int seconds)
{
    const double start = second();

    unsigned long best_solution_starting_node = rand() % get_number_of_nodes(solution->instance);
    solve_instance(solver, best_solution_starting_node, solution->tour, &solution->cost, solution->instance);
    double best_solution_cost = solution->cost;

    while (second() < start + seconds)
    {
        solve_instance(solver,
                       rand() % get_number_of_nodes(solution->instance),
                       solution->tour,
                       &solution->cost,
                       solution->instance);

        if (solution->cost < best_solution_cost)
        {
            best_solution_starting_node = solution->tour[0];
            best_solution_cost = solution->cost;
        }
    }

    solve_instance(solver, best_solution_starting_node, solution->tour, &solution->cost, solution->instance);

    return check_solution_feasibility(solution);
}

void free_tsp_solution(TspSolution* solution)
{
    if (!solution || !solution->tour)
    {
        return;
    }
    free(solution->tour);
    solution->tour = NULL;
    free(solution);
}
