# CTransportationPlannerCommandLine
This class is for handling user input from the command line. It basically run a big loop that wait for user to type something, then it call the Planner to do things like finding paths or counting nodes.

## Public Interface
CTransportationPlannerCommandLine(std::shared_ptr<CDataSource> cmdsrc, ...)

Constructor. It takes a lot of things like where to read commands (cmdsrc), where to write output (outsink), and the planner itself. It just init the internal SImplementation.

### bool ProcessCommands()

This is the main function. It start a loop that read lines one by one until user type exit or the source end.

## Commands it Support
When you run the program, you can type these things:

### help: Print a long message showing all commands.

### count: Ask the planner how many nodes in map and print it.

### node [index]: Give an index, and it print node ID and its location (like 38d 12'...).

### shortest [start] [end]: Calculate distance for shortest path between two node IDs.

### fastest [start] [end]: Calculate time for fastest path. It print it in hr/min/sec format.

### print: If you just calculated a path, this will print the "human instructions" like "Walk along Main St".

### ave: Save the last path into a .csv file in the <results> folder.

### exit: Stop the loop and close program.

## Code example
```cpp
// created in main.cpp
CTransportationPlannerCommandLine cmd_line(source, out, err, factory, planner);

// start the loop and wait for user
cmd_line.ProcessCommands();
```