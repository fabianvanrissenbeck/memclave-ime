# Dealing with Threads While Loading new Blocks

Multiple threads being active can cause significant issues for
the integrity of the core loader. It therefore has to be able to
realiably determine whether other threads are alive.

**Available Threading Instructions and Assumptions**

`boot` sets the pc of the specified thread to 0, if that thread
has no instruction being processed by the pipeline. The thread
continous or starts execution at address 0. `boot` performs
this action only if the thread's run bit is zero. After finishing,
the thread's run bit is 1. `boot` can branch based on the old
value of the thread's run bit.

`clr_run` can set the run bit of one thread to zero. This does not
stop the thread. The instruction `clr_run` branches based on
the old value of the thread's run bit.

`resume` is like boot, except that the pc is not set to 0.

`stop` clears the run bit of the calling threat and stop it. After
stopping it sets the pc to some immediate value.

**Figuring out Running Threads**

+ Implement the following policy: All threads, when stopping, stop
  to one specific address `__thread_stop`. This address is not equal
  to `__bootstrap`.
  + When issueing a `boot` call to a thread, this thread starts at
    `__bootstrap`. When issueing `resume`, it starts at `__thread_stop`.
+ Restrict use of all threading instruction but `stop` to only the core
  loader. No other component may call threading related instructions.
+ Additionally, only thread 0 may call threading related instructions
  other than `stop`.
+ Do not call `clr_run` anywhere. The only way to clear a run bit
  becomes self termination.
+ Test thread activity by calling `boot` on all threads. `__bootstrap`
  is implemented in a way, that causes all threads but the first one
  to stop to `__thread_stop` almost immediately.
  + This causes all run bits to be raised, independent of their prior
    value.
  + All threads that were actually dead, die almost immediately again.
    This clears their run bit.
  + Threads that weren't dead but not in the pipeline also die.
  + Threads that are active won't die. If their run bit was lowered
    before, it was raised again using the boot instruction.
  + Inspect the run bits by calling boot again on all threads. If their
    prior state is running, they were active before. If it is not 
    running, they were either successfully killed or not active in the
    first place.