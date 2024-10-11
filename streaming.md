# A Compiler For Streaming Algorithms

We want utilize SIMD instructions  to process process parallel streams of data.
In our context, a stream is an ordered sequence of data points, and we want to
apply the same algorithm to all the streams.


## Use supplied algorithm

The user provides an algorithm that consume the head of a single stream a state,
and returns an output and an updated state:

```
algo = (T, State) → (S, State)
```

The user-supplied algorithm can call as subroutines other algorithms that
similarly operate on a stream. For syntactic convenience, however,  any mention
of the state is elided from the call to subroutines. So for example, the
user-defined function to compute the running mean of a steam might formally
track the state of an accumulator and a counter, and pass these to subrountines,
and would look quite verbose if it had to explicitly track the state of all the
subroutines it calls:

```
mean(x, state):
  cnt, state.count = count(x, state.count)
  tally, state.sum = sum(x, state.sum)
  return (tally/cnt, state)
```

Instead, we elide all the state variables and can just write

```
mean(x) = sum(x) / count(x)
```

The compiler recognizes that both sum and count need to maintain state, so it
allocates a state variable for each of them, and automatically passes the state
to each of them as needed.

## Process multiple streams in parallel

Given a group of streams s1...sn, we could naively apply the user-defined
algorithm to each stream sequentially. But our job is to compile the algorithm
into one that can process multiple unsynchronized streams in parallel, using all
the parallelism the machine offers. By "unsynchonized" I mean that one stream
might receive a burst of updates while another input stream is stalled.  At each
time step during runtime, the program receives a packet of data. This packet
contains updates to any number of streams.

Our job is to convert  a user-supplied function definedo on a single stream to
one that  can operate on multiple streams in parallel, with the 
signature:

```
algo_multi_stream = Stream (stream_id, T, State), algo → Stream (stream_id, T, State)
```

Here, a packet is itself represented as a stream, each element of which
indicates the stream_id for which the packet contains a update.

We'll compile this function down to a function that maintains the state for all
streams, then at each time step, mutates the state of the streams mentioned in
the packet:

```
compiled_algo_multi_stream = Stream (stream_id, T), State[max_streams] → (Stream (stream_id, T), State[max_streams])
```

This compiled function uses SIMD instructions to gather and modify the enetries
of the state vector. Since `algo` can be recursively defined in terms of other
`algos`, `algo_multi_stream` will similarly depend recursively on other
`algo_multi_stream`s and so too can `compiled_algo_multi_stream`s.


## Doing all of this in C++

Initially, I thought I might need to write a compiler for this from the ground
up, but it turned out to be straightforward to ddo all of this in C++.

The compile boils down to a simple trick: When the user supplies `algo`, they do
so in C++, using special scalar stub types that actually stand for vectors of
scalars.  This way, `algo` can be applied to small vectors natively.  generating
`compiled_algo_multi_stream`, the compiler calls `algo` on a gathered
subset of the state vector, and scatters the results back to the state vector.