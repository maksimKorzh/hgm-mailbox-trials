# Mailbox trials by H.G. Muller
I opened this thread to conduct a sort of blog on comparing the speed of<br>
several experimental mailbox algorithms. The speed comparison of the various<br>
techniques will be done for a toy 'model engine', which uses a fixed-depth search<br>
plus capture-only quiescence (including delta pruning), with MVV/LVA sorting of the captures,<br>
and an evaluation of PST plus (optionally) mobility.<br>
No hash table, no killer or history heuristic,<br>
so basically no particuar move ordering of the non-captures;<br>
the time-to-depth is not really of interest here, we will be comparing nodes per second.<br>
To not lose ourselves in details, promotion, castling and e.p. capture will be omitted.<br>
The speed will be measured in a search of the KiwiPete position.<br>
<br>
r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1<br>
<br>
The first design to be tested will be the 16x12 mailbox + piece list.<br>
We should already have a rough idea how this works, because<br>
<a href="https://home.hccnet.nl/h.g.muller/perft.c">Qperft* (*super fast perft by H.G. Muller - note by CMK)</a> uses this design.<br>
It does a perft(6) of KiwiPete in 45 sec, with bulk counting.<br>
That means it has done move generation in perft(5) nodes (193M nodes), which translates to 4.26 Mnps.<br>
But because that is perft there are two effects that slow it down compared to a search:<br>
there is some legality testing (on King moves), which a search based on pseudo-legal moves would not do.<br>
But, more importantly, in perft all nodes are 'full width', while in a search most nodes are QS nodes.<br>
Refraining from putting the non-captures in a move list would speed up move generation appreciably.<br>
And QS nodes that experience a stand-pat cutoff will not have to generate moves at all.<br>
(At least, if the evaluation doesn't include mobility.)<br>
So the search should be significantly faster, perhaps even double.<br>
<br>
We will see shortly...<br>
<a href="http://talkchess.com/forum3/viewtopic.php?f=7&t=76773">read more...</a>

# How to compile it with GCC
    gcc -Ofast mailbox7.c -o mailbox7
