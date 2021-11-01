# Initialization oder
## Goal
Due to the usage of global objects with their own constructors, it's necessary to make some kind of initialization order to resolve the dependency problems.
Even though there is compiler supported attributes to handle such cases (\_\_attribute\_\_ ((init\_priority (X)))), it is not working on template generated static objects.
## Solution
### Theory
Since the objects' constructors' order is not settable, they should contain minimal code, all dependency related initialization should be done elsewhere later.
### Interface
The class Object has a virtual init() and a virtual initPriority() method to meet this requirement.
Static objects that descendentants of Object are chained to list during construction and later their init() methods are called in the proper order.
### Order of initialization
The current Object descendant classes and their initialization order:
 1. Task (TaskRoot) - 0
 1. Buffer (DataBuffer) - 10
 1. Provider - 20

