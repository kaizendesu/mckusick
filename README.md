## How to Read Operating System Code Effectively

1. Create a flow diagram of all the relevant functions and their subroutines
in the order that they are executed. The flow diagram will have a depth of
four layers, with each layer describing a stack frame/context.

2. Create the code reading checklist that will list every relevant function
we want to read by filename and in the order they are called within that
file. Create four dashes "----" to with the following meanings:
	* The first check means I have read the function and have an idea of
      what it does.
	* The second check means that I have heavily commented the function.
	* The third check means that I have read through the code again with
      a focus on the bigger picture and anything I may have overlooked.
	* The fourth check means I have added the commented code to the walkthrough.
	
3. Create the "Important Data Structures and Definitions" section by scanning
the code for the most used data structures, identifying which header files they
are defined in, reading the definitions of said structures, and also reading
any detailed comments describing the structures and the overall system.

4. Create the Code Walkthrough Overview section which describes in English
what each function in the flow diagram does without any focus on the actual
implementation. A summary for each function in the flow diagram MUST be
completed before moving on to the fifth step.

5. Read through the code line by line focusing on how each function accomplishes
what we wrote in step four. Use the checklist to monitor your progress since
code reading is not always a linear process.

6. Decide on which functions to include in the walkthrough document and copy
them to it.