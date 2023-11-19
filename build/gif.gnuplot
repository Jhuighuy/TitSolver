#*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*#

#set terminal gif animate size 1600,1200 delay 0
set terminal gif animate size 1920,1080 delay 0
set output 'velocity.gif'

#set yrange [-2.1:2.1]
#set xrange [-0.1:6.1]
set xrange [-0.1:3.6]
set yrange [-0.1:2.6]
#set xrange [-0.1:4.1]
#set yrange [-0.1:3.1]
set cbrange [-0.05:1.05]
#set cbrange [-3000:3000]

#*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*#
# MATLAB jet color palette

# line styles
set style line 1 lt 1 lc rgb '#000080' #
set style line 2 lt 1 lc rgb '#0000ff' #
set style line 3 lt 1 lc rgb '#0080ff' #
set style line 4 lt 1 lc rgb '#00ffff' #
set style line 5 lt 1 lc rgb '#80ff80' #
set style line 6 lt 1 lc rgb '#ffff00' #
set style line 7 lt 1 lc rgb '#ff8000' #
set style line 8 lt 1 lc rgb '#ff0000' #
set style line 9 lt 1 lc rgb '#800000' #
# line style used together with jet (<2014b)
set style line 11 lt 1 lc rgb '#0000ff' # blue
set style line 12 lt 1 lc rgb '#007f00' # green
set style line 13 lt 1 lc rgb '#ff0000' # red
set style line 14 lt 1 lc rgb '#00bfbf' # cyan
set style line 15 lt 1 lc rgb '#bf00bf' # pink
set style line 16 lt 1 lc rgb '#bfbf00' # yellow
set style line 17 lt 1 lc rgb '#3f3f3f' # black

# palette
set palette defined \
  ( 0  0.0 0.0 0.5, \
    1  0.0 0.0 1.0, \
    2  0.0 0.5 1.0, \
    3  0.0 1.0 1.0, \
    4  0.5 1.0 0.5, \
    5  1.0 1.0 0.0, \
    6  1.0 0.5 0.0, \
    7  1.0 0.0 0.0, \
    8  0.5 0.0 0.0  )

#*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*#

set key autotitle columnhead

min(x,y) = x < y ? x : y
max(x,y) = x > y ? x : y
clamp(a,mn,mx) = max(min(a,mx),mn)

H = 0.6
#H = 2.0

do for [i=0:100000000] {
	plot 'output/test_output/particles-'.i.'.csv' \
       u 'r_x':'r_y':(clamp(column('p')/(1000*9.81*H), -0.1, 1.5)) \
       w p pt 7 palette notitle
}
