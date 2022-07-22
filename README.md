# Linker
A programming lab with professor Hubertus Franke. 

We use a two-pass linker in this lab. By resolving external symbol references (such as variables and functions) and module relative addressing by assigning global addresses after placing the modules' object code at global locations, a linker takes individually built code/object modules and builds a single executable.  

We assume a target machine with the following characteristics rather than dealing with intricate x86 tool chains:  
(1) word addressable (2) addressable memory of 512 words  (3) each valid word is represented by an integer (&lt;10000).
