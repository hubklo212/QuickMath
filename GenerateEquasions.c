#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NUM 10   // Maximum value for operands in the equations
#define MIN_NUM 1    // Minimum value for operands in the equations

// Function to generate a random integer between min and max (inclusive)
int generateRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Function to generate a random operator (+, -, *)
char generateRandomOperator() {
    char operators[] = {'+', '-', '*'};
    int index = rand() % 3;
    return operators[index];
}

// Function to generate a random simple equation as a string
char* generateRandomEquation() {
    char* equation = malloc(5 * sizeof(char));  // Assuming maximum equation length is 5
    int operand1 = generateRandomNumber(MIN_NUM, MAX_NUM);
    int operand2 = generateRandomNumber(MIN_NUM, MAX_NUM);
    char operator = generateRandomOperator();

    snprintf(equation, 20, "%d%c%d", operand1, operator, operand2);

    return equation;
}

// Function to evaluate a simple equation and return the result
int evaluateEquation(char* equation) {
    int operand1, operand2, result;
    char operator;

    sscanf(equation, "%d%c%d", &operand1, &operator, &operand2);

    switch (operator) {
        case '+':
            result = operand1 + operand2;
            break;
        case '-':
            result = operand1 - operand2;
            break;
        case '*':
            result = operand1 * operand2;
            break;
        default:
            printf("Invalid operator\n");
            exit(EXIT_FAILURE);
    }

    return result;
}

// Function to generate an array of random equations and their results
char** generateRandomEquations(int numEquations) {
    char** equationsAndResults = malloc(2 * numEquations * sizeof(char*));  // Allocate space for equations and results
    srand(time(NULL));

    for (int i = 0; i < 2 * numEquations; i += 2) {
        equationsAndResults[i] = generateRandomEquation();
        equationsAndResults[i + 1] = malloc(3 * sizeof(char));  // Assuming maximum result length is 3
        snprintf(equationsAndResults[i + 1], 20, "%d", evaluateEquation(equationsAndResults[i]));
    }

    return equationsAndResults;
}

// Function to free memory allocated for equations and results
void freeEquations(char** equationsAndResults, int numEquations) {
    for (int i = 0; i < 2 * numEquations; ++i) {
        free(equationsAndResults[i]);
    }
    free(equationsAndResults);
}


/*
int main() {
    int numEquations = 20;  // Change this value to control the number of equations
    char** equationsAndResults = generateRandomEquations(numEquations);

    // Print the generated equations and results
    for (int i = 0; i < 2 * numEquations; i += 2) {
        printf("%s=%s\n", equationsAndResults[i], equationsAndResults[i + 1]);
    }

    // Free the allocated memory
    freeEquations(equationsAndResults, numEquations);

    return 0;
}
*/

