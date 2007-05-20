
# ruby satori_sources.rb|zip -@ -v9 satori122source.zip

target = ARGV.shift
src = {}
dsp = []

Dir.open(".\\#{target}").each{|i|
	case i
	when /\.dsp$/
		dsp.push i
		src["#{target}\\#{i}"] = true
	when /\.dsw$/
		src["#{target}\\#{i}"] = true
	when /^makefile\./
		src["#{target}\\#{i}"] = true
	end
}

dsp.each{|i|
	fh = open "#{target}\\#{i}"
	fh.each{|j|
		next if j !~ /^SOURCE=(.+)/
		path = $1.chop
		path.sub! /\.\.\\/, ''
		path.sub! /\.\\/, "#{target}\\"
		src[path] = true
	}
	fh.close
}

src.keys.each{|i|
	print "#{i}\n"
}

