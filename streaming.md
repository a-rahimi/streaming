# A Compiler For Streaming Algorithms

We want to process parallel streams of data on a multi-core machine, and utilize
the SIMD instructions of each core to process these streams. In our context, a
stream is an ordered sequence of data points, and we want to apply the same
algorithm to all the streams.

```
Stream T = T ▶ Stream T
```

The user provides an algorithm that consume one data and a state, and returns an
output and an updated state. 

```
algo = (T, State) → (S, State)
```

This algorithm can call as subroutines other algorithms that similarly operate
on a stream. For syntactic convenience, however,  any mention of the state is
elided from the call to subroutines. The compiler automatically initializes the
state for each stream and each subroutine, and inserts the state variables.
For example, suppose we had a function that counts the number of elements in a
stream so far, and another function that accumulates the sum of all elements in
a stream so far:

```
count = (T, int) → (int, int)
count(x, state) = (state + 1, state + 1)

sum = (T, T) → (T, T)
sum(x, state) = (state + x, state + x)
```

Then the algorithm thta computes the mean of a stream can be written as:

```
mean = (T, State) → (T, State)
mean(x, state) = sum(x) / count(x)
```

The compile recognizes that both sum and count need to maintain state, so it
allocates a state variable for each of them, and automatically passes the state
to each of them as needed.

Given a group of streams s1...sn, we could naively apply the user-defined
algorithm to each stream sequentially. But our job is to compile the algorithm
into one that can process multiple ragged streams in parallel, using all the
parallelism the machine offers. By "ragged" I mean that the incoming streams
need not be synchronized.  One stream might receive a burst of updates while
another input stream is stalled.  At each time step during runtime, the program
receives a packet of data. This packet contains updates to any number of
streams. So in effect, our job is to generate a function with the following
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
`algo_multi_stream`s and so too can `comiled_algo_multi_stream`s.

The compile boils down to a simple trick: When the user supplies `algo`, they do
so in C++, using special scalar stub types that actually stand for vectors of
scalars.  This way, `algo` can be applied to small vectors natively. See
[Tuesday](https://github.com/Cincinesh/tue) for an example of how this is done.
Then when
generating `compiled_algo_multi_stream`, the compiler calls `algo` on a gathered
subset of the state vector, and scatters the results back to the state vector.