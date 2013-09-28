fun size {
	let $n = 0
	while fetch $e {
		let $n = ($n + 1)
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
		let $i = ($i + 1)
	}
}

fun enumerate {
	let $i = 0 
	while fetch $e {
		yield $i $e
		let $i = ($i + 1)
	}
}

fun all {
	while fetch $e {
		if ($e == 0) {
			return 0
		}
	}
	return 1
}

fun any {
	while fetch $e {
		if ($e != 0) {
			return 1
		}
	}
	return 0
}

fun testLet {
	yield "test let"
	let name = name || yield NG
	let name = Name && yield NG
	let $var = name || yield NG
	let $var $var = name && yield NG
	let ($var) = name name || yield NG
	let $var name = $var || yield NG
}

fun testDefer {
	defer echo
	range 8 | while fetch $i {
		echo -n $i
		defer echo -n $i
	}
	echo
}

fun factorialLoop $n {
	let $result = 1
	range $n | while fetch $i {
		let $result = ($result * ($i + 1))
	}
	yield $result
}

fun factorialRec $n {
	if ($n <= 1) {
		yield 1
	}
	else {
		yield ($n * [factorialRec ($n - 1)])
	}
}

fun ackermann $m $n {
	if ($m == 0) {
		yield ($n + 1)
	}
	else if ($n == 0) {
		ackermann ($m - 1) 1
	}
	else {
		ackermann ($m - 1) [ackermann $m ($n - 1)]
	}
}

fun qsort0 ($xs) {
	if let $pv ($xs) = $xs {
		qsort0 [$xs -> while fetch $x {
			if ($x <= $pv) {
				yield $x
			}
		}]
		yield $pv
		qsort0 [$xs -> while fetch $x {
			if ($x > $pv) {
				yield $x
			}
		}]
	}
}

fun qsort1 {
	if fetch $pv ($xs) {
		$xs -> while fetch $x {
			if ($x <= $pv) {
				yield $x
			}
		} | qsort1
		yield $pv
		$xs -> while fetch $x {
			if ($x > $pv) {
				yield $x
			}
		} | qsort1
	}
}

fun redirect {
	test.rs >| cat | wc
}

fun returnInPipe {
	range 16 | while fetch $i {
		yield $i
		if ($i == 8) {
			return 0
		}
	}
	yield "last stmt"
}

fun nested {
	let $p = "OK"
	let $q = "BAD"
	let $r = "OK"
	fun inner $r {
		let $q = "OK"
		yield $p
		yield $q
	}
	inner "BAD"
	yield $r
}

fun runBg {
	let $t = [& { sleep 2 ; yield "finished thread" }]
	sys.join $t
	yield "finished join"
}

fun testSlice {
	let ($arr) = [range 8]
	echo $arr[0]
	echo $arr[0 2 4 8]
	echo $arr[0 * 0 + 2]
	echo $arr[ 0 :  2]
	echo $arr[ 0 : -1]
	echo $arr[-1 :  0]

	let ($err) =
	! echo $err[0] &&
	! echo $err[ 0 :  2] &&
	! echo $err[ 0 : -1] &&
	! echo $err[-2 : -1] &&
	echo "slice OK"
}

fun emulPrevNext {
	let $cwDir = "/0"

	fun std.pwd {
		yield $cwDir
	}

	fun std.cd $dir {
		let $cwDir = $dir
		echo $dir
	}

	let ($dir_prev) =
	let ($dir_next) =

	fun cd $dir {
		let $cwd = [std.pwd]
		if std.cd $dir {
			let ($dir_prev) = $dir_prev $cwd
			let ($dir_next) =
		}
	}

	fun nd {
		if let ($dir_next) $dir = $dir_next {
			let ($dir_prev) = $dir_prev [std.pwd]
			std.cd $dir
		}
	}

	fun pd {
		if let ($dir_prev) $dir = $dir_prev {
			let ($dir_next) = $dir_next [std.pwd]
			std.cd $dir
		}
	}

	echo [
		cd /1
		cd /2
		cd /3
		pd
		cd /4
		pd
		pd
		nd
		range 16 | while fetch $i {
			pd
		}
	]
	echo /1 /2 /3 /2 /4 /2 /1 /2 /1 /0
}

fun testZip {
	zip || yield "NG"
	zip ()
	zip (0 1 2) (a b c)
}

fun localCwd {
	chdir ~
	pwd
	chdir /
	pwd
}

fun testDivMod {
	let ($as) = (+13 -13 +13 -13 +20 -20 +20 -20)
	let ($bs) = (+10 +10 -10 -10 +10 +10 -10 -10)
	echo ($as / $bs == +1 -2 -2 +1 +2 -2 -2 +2)
	echo ($as % $bs == +3 +7 -7 -3  0  0  0  0)
	echo (2 * 3 / 3 * 2 == 1)
}

fun runTest {
	yield (1 6 + 2 1 * 3 1)
	echo "args: " $args
	let $var = S
	echo P(a b c)[range 4]$var
	testLet
	factorialRec 16
	factorialLoop 16
	ackermann 3 2
	echo 29
	testSlice
	testDefer
	range 16 | index 4
	range 16 | slice 4 6
	redirect
	echo [qsort0 0 3 2 6 -10 12312 23 2 98 8]
	echo [0 3 2 6 -10 12312 23 2 98 8 -> qsort1]
	returnInPipe
	nested
	runBg // comment
	emulPrevNext
	echo [testZip]
	//comment

	if true { // comment
		true
	} // comment
	else {
		//comment //comment
	}
	localCwd

	testDivMod
}

runTest
