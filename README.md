# taskpool
Builds a directed acyclic graph of tasks with logical interdependencies and implements a threadpool to complete them concurrently with respect to the dependencies.

Dependencies currently support a combination of logical AND and logical OR. Currently tasks are simple void functions but this can and will be expanded eventually to have tasks manipulate a shared resource, for example could be used to perform parallel computation of complex arithmetic or used to install dependencies for another program ala package.json for npm.
