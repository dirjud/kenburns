import numpy, pylab

x0    = 100.0
y0    = 100.0
z0    = 40.0

thetas = [  -10,  0,  45,   60 ]
colors = [  'm', 'c', 'r', 'g', 'b' ]

pylab.figure(figsize=[12,11])
pylab.plot([0, 0], [0,y0], 'k')
pylab.plot([-z0,0],[0,y0], 'k')
pylab.plot([-z0,0],[0, 0], 'k')

for theta1, color in zip(thetas, colors):

    theta = theta1 / 180.0 * numpy.pi
    tan_theta = numpy.tan(theta)
    
    z1 = z0 * z0 / (z0 + y0 * tan_theta)
    y1 = z0 * y0 / (z0 + y0 * tan_theta)
    
    y2 =  y1*cos(-theta) + (z1-z0)*sin(-theta)
    z2 = -y1*sin(-theta) + (z1-z0)*cos(-theta)


    pylab.plot([0, z1-z0],[0,y1], color)
    pylab.plot([-z0, z1-z0],[0,y1], color)
    pylab.plot([z2],[y2],color+'o')

d  =  200
xl = -100
yl = -5
pylab.xlim([xl,xl+d])
pylab.ylim([yl,yl+d])

#pylab.plot([-z0,-z0+z1], [0,0])
pylab.grid(True)


pylab.figure()
y0 = numpy.linspace(0,220, 100)
theta = -10.0 / 180.0 * numpy.pi
tan_theta = numpy.tan(theta)
    
z1 = z0**2   / (z0 + y0 * tan_theta)
y1 = z0 * y0 / (z0 + y0 * tan_theta)
    
y2 =  y1*cos(-theta) + (z1-z0)*sin(-theta)
z2 = -y1*sin(-theta) + (z1-z0)*cos(-theta)


pylab.plot(y0, y2)
pylab.grid(True)


pylab.figure(figsize=[12,11])
y1 = numpy.linspace(-100,100,201)
x1 = 100
z1 = 100
yrot = 0
thetay = yrot / 180.0 * numpy.pi
tan_thetay = numpy.tan(thetay)

z2 = z1 * z1 / (z1 + y1 * tan_thetay)
y2 = z1 * y1 / (z1 + y1 * tan_thetay)
x2 = x1 * z2 / z1

pylab.plot( [-z1, 0 ], [ 0, y1[0] ],  'k')
pylab.plot( [-z1, 0 ], [ 0, y1[-1] ], 'k')
pylab.plot( z2, y2, '.-r')

for xrot in [ 110, 70 ]:
    thetax = xrot / 180.0 * numpy.pi
    tan_thetax = numpy.tan(thetax)
    
    z3 = z2 * z2 / (z2 + x2 * tan_thetax)
    x3 = x2 * z2 / (z2 + x2 * tan_thetax)
    y3 = y2 * z3 / z2

    
d  =  1000
xl = -d/2
yl = -d/2
pylab.xlim([xl,xl+d])
pylab.ylim([yl,yl+d])
pylab.grid(True)
