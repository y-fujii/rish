fun size {
	let $n = 0
	while fetch $e {
		let $n = $n + 1
	}
	yield $n
}

fun index $n {
	enumerate | while fetch $i $e {
		if ($i == $n) {
			yield $e
			break 0
		}
	}
}

fun slice $bgn $end {
	enumerate | while fetch $i $e {
		if ($bgn <= $i) && ($i < $end) {
			yield $e
		}
	}
}

fun range $n {
	let $i = 0
	while ($i < $n) {
		yield $i
		let $i = $i + 1
	}
}

fun enumerate {
	let $i = 0 
	while fetch $e {
		yield $i $e
		let $i = $i + 1
	}
}

fun select ($cs) {
	enumerate | while fetch $i $e {
		if $cs($i) {
			yield $e
		}
	}
}

fun all {
	while fetch $e {
		if ! $e {
			return -1
		}
	}
	return 0
}

fun any {
	while fetch $e {
		if $e {
			return 0
		}
	}
	return -1
}

fun min {
	fetch $r || return -1
	while fetch $x {
		if ($x < $r) {
			let $r = $x
		}
	}
	yield $r
}

fun max {
	fetch $r || return -1
	while fetch $x {
		if ($x > $r) {
			let $r = $x
		}
	}
	yield $r
}

fun sum {
	let $s = 0
	while fetch $x {
		let $s = $s + $x
	}
	yield $s
}

fun abs $x {
	if ($x < 0) {
		yield (- $x)
	}
	else {
		yield (+ $x)
	}
}

fun sqrt $x {
	if ($x == 0) {
		yield 0
		return
	}

	let $t = $x
	while 0 {
		let $u = ($t * $t + $x) / 2 * $t
		if ($u >= $t) {
			break
		}
		let $t = $u
	}
	yield $u
}

let $randomX = 123456789
let $randomY = 362436069
let $randomZ =  77465321
let $randomC =     13579

fun random {
	// MWC PRNG, G. Marsaglia
	let $t = 916905990 * $randomX + $randomC
	let $randomX = $randomY
	let $randomY = $randomZ
	let $randomC = $t / 4294967296
	let $randomZ = $t % 4294967296
	yield $randomZ
}

fun random:choice {
	fetch ($xs)
	yield $xs([random])
}

fun qsort {
	fetch ($xs)
	if (#xs > 0) {
		let $pv = $xs([random])
		$xs -> while fetch $x { if ($x <  $pv) { yield $x } } | qsort
		$xs -> while fetch $x { if ($x == $pv) { yield $x } }
		$xs -> while fetch $x { if ($x >  $pv) { yield $x } } | qsort
	}
}

fun msort {
	fetch ($xs)
	// XXX: bad spec.
	if (#xs <= 1) {
		yield $xs
		return
	}

	let ($fst) = [$xs(0 : #xs / 2) -> msort]
	let ($snd) = [$xs(#xs / 2 : 0) -> msort]

	let $i $j = 0 0
	while ($i < #fst) && ($j < #snd) {
		if ($fst($i) <= $snd($j)) {
			yield $fst($i)
			let $i = $i + 1
		}
		if ($fst($i) >= $snd($j)) {
			yield $snd($j)
			let $j = $j + 1
		}
	}
	if ($i < #fst) {
		yield $fst($i : 0)
	}
	if ($j < #snd) {
		yield $snd($j : 0)
	}
}

fun uniq {
	fetch $x0 || return
	yield $x0
	while fetch $x1 {
		if ($x0 != $x1) {
			yield $x1
			let $x0 = $x1
		}
	}
}

fun diff ($ys) {
	while fetch $x {
		if ($x != $ys) -> all {
			yield $x
		}
	}
}

fun isect ($ys) {
	while fetch $x {
		if ($x == $ys) -> any {
			yield $x
		}
	}
}

fun union ($ys) {
	diff $ys
	yield $ys
}

fun contain ($ys) {
	fetch ($xs)
	$ys -> while fetch $y {
		if ($y != $xs) -> all {
			return -1
		}
	}
	return 0
}

fun assoc $key {
	while fetch $k $v {
		if ($k == $key) {
			yield $v
		}
	}
}

fun remove $key {
	while fetch $k $v {
		if ($k != $key) {
			yield $k $v
		}
	}
}

fun testStd {
	let ($xs) = [range 64 | while fetch $_ { random }]
	([$xs -> msort] == [$xs -> qsort]) -> all || yield NG
	([1 1 2 3 3 3 4 4 5 6 8 -> uniq] == 1 2 3 4 5 6 8) -> all || yield NG
	([0 1 2 3 4 5 6 -> diff 1 3 5 7 9] == 0 2 4 6) -> all || yield NG
	([0 1 2 3 4 5 6 -> select (0 1 2 3 4 5 6 % 2 == 0)] == 0 2 4 6) -> all || yield NG
}
