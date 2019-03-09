# Explicit-LinkedList-Memory-Allocator
Memory allocator written in C. This implementation make uses of the explicit linked list structure to store and manage memory in heap. 
This memory allocator performs at approximately 76% throughput in comparison to the GNU implementation.

## Getting Started
This is a partial code display for a bigger project and is not meant to be run on your local machine. 
Helper functions have been excluded from the upload to prevent possible plagarism for school projects.

## Current Features
 - Chunk design: header (size and status + padding), payload, footer. 
 - Realloc: changes the size of a block.
 - Split: splits free block into two partitions.
 - Coalesce: stitch up neighboring free chunks.
 - CheckHeap: check for heap consistency and detect any empty chunks.
 
## Built With
Built in C.
 
## Author(s)
 Derick Chen Yuran - [LinkedIn](https://www.linkedin.com/in/derick-chen-a672866b/)

## Other Notes
Nil
