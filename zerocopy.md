# implmenting zerocopy to optmise network performance

Network performance is limited by amonst other things CPU overhead at the interface between user and kernel. Two aspects of this which are candidates for optimisation are the frequency of system calls, and the overhead associated with each call. Setting larger blocksizes for transfers at each system call directly reduces the number of system calls required to complete an operation, with direct proportionate benefits. Avoiding copying data bwteen kernel and userspace is another potential way to reduce overhead.
Whilst there is no generic implementation of zerocopy between user and kernel space for system calls, there is one specific case which is supported - the 'sendfile()' system call. This allows the kernel to source data from an input and transfer it to a sink without the data ever being copied into and out ofuser space, hence acheieving a 'zerocopy' operation.  The simple use case would be transmission of the content of a real disk file, however, for purposes of testing a network connection, having to use a 'real' file would itself represent a potential performance constraint.  But the source for sendfile() can be any 'mmapable' device, whcih includes files which are already mapped into memory, or even sparse files, as long as sending zeros is adequate for the purpose!  So no physical input IO need be performed in order to fulfil a sendfile operation.
## Implementation
For arbitrary payload we do need a physical file, however, if mmapped and locked we can be assured that no acual file IO need be performed in order to send it's contents. An additional tweak is to specify a sparse file, which enables zero blocks to be sent without even allocating either memory or disk space for it.
The implmentation of this requires the following steps:
* create the temporary file (open/mkstemp)
* (sparsely) extend the file to required length (ftruncate)
* use the file in the sendfile() call
