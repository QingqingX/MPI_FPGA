Ultra-low latency communication in HPC is difficult to achieve because of MPI processing requirements, in particular matching requests and
messages done by traversing the corresponding queues. Many researchers have addressed this issue by redesigning queues or by offloading them to hardware accelerators. However, state-ofart software approaches cannot free CPUs “from the misery”
and hardware approaches either lack scalability or still leave substantial room for further improvement.

With the emergence of numerous tightly coupled CPU-FPGA computing architectures, offload of MPI functionality to usercontrolled hardware is now becoming viable; we find it productive
to revisit hardware approaches. To maintain the generality necessary to support MPI while preventing high resource utilization, we design our MPI queue processing offload based
on a recent analysis of performance characteristics in HPC applications. We propose a novel, two-level message queue design: a content addressable memory (CAM) coupled with a resourcesaving hardware linked-list. We also propose an optimization
that maintains high speed in the cases when the queue is long. To test our design, we create an SOC-based testbed consisting of
softcore processors and hardware implementations of the MPI communication stacks. Even while using only a small fraction of the Stratix-V logic, our design can be one to two orders of magnitude faster than two well-known hardware designs.
