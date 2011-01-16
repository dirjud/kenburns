import numpy, pylab

# x0    = 100.0
# y0    = 100.0
# z0    = 40.0
# 
# thetas = [  -10,  0,  45,   60 ]
# colors = [  'm', 'c', 'r', 'g', 'b' ]
# 
# pylab.figure(figsize=[12,11])
# pylab.plot([0, 0], [0,y0], 'k')
# pylab.plot([-z0,0],[0,y0], 'k')
# pylab.plot([-z0,0],[0, 0], 'k')
# 
# for theta1, color in zip(thetas, colors):
# 
#     theta = theta1 / 180.0 * numpy.pi
#     tan_theta = numpy.tan(theta)
#     
#     z1 = z0 * z0 / (z0 + y0 * tan_theta)
#     y1 = z0 * y0 / (z0 + y0 * tan_theta)
#     
#     y2 =  y1*cos(-theta) + (z1-z0)*sin(-theta)
#     z2 = -y1*sin(-theta) + (z1-z0)*cos(-theta)
# 
# 
#     pylab.plot([0, z1-z0],[0,y1], color)
#     pylab.plot([-z0, z1-z0],[0,y1], color)
#     pylab.plot([z2],[y2],color+'o')
# 
# d  =  200
# xl = -100
# yl = -5
# pylab.xlim([xl,xl+d])
# pylab.ylim([yl,yl+d])
# 
# #pylab.plot([-z0,-z0+z1], [0,0])
# pylab.grid(True)
# 
# 
# pylab.figure()
# y0 = numpy.linspace(0,220, 100)
# theta = -10.0 / 180.0 * numpy.pi
# tan_theta = numpy.tan(theta)
#     
# z1 = z0**2   / (z0 + y0 * tan_theta)
# y1 = z0 * y0 / (z0 + y0 * tan_theta)
#     
# y2 =  y1*cos(-theta) + (z1-z0)*sin(-theta)
# z2 = -y1*sin(-theta) + (z1-z0)*cos(-theta)
# 
# 
# pylab.plot(y0, y2)
# pylab.grid(True)


pylab.figure(figsize=[12,11])
y1 = numpy.array([-100, -90, -80, 0, 100]) #numpy.linspace(-100,100,201)
x1 = 0
z1 = 554.256
W = 100
H = 50
yrot = 10
xrot = 0
Tx = xrot / 180.0 * numpy.pi
Ty = yrot / 180.0 * numpy.pi

det = -x1 * sin(Tx) + y1 * cos(Tx) * sin(Ty) + z1 * cos(Tx) * cos(Ty)
print det
G   = cos(Ty)*cos(Tx)

x2 = G/det * x1 * z1
y2 = G/det * y1 * z1
z2 = G/det * z1 * z1 - z1

#Ty=0 # 3.14159/2-Ty
x3 =  x2*cos(Tx) + y2*sin(Ty)*sin(Tx) + z2*cos(Ty)*sin(Tx)
y3 =             + y2*cos(Ty)         - z2*sin(Ty)
z3 = -x2*sin(Tx) + y2*sin(Ty)*cos(Tx) + z2*cos(Ty)*cos(Tx)

pylab.plot( [-z1, 0 ], [ 0, y1[0] ],  'k')
pylab.plot( [-z1, 0 ], [ 0, y1[-1] ], 'k')
pylab.plot( numpy.zeros_like(y1), y1, '.-g')
pylab.plot( z2, y2, '.-r')
pylab.plot( z3, y3, '.-b')

for xrot in [ 110, 70 ]:
    thetax = xrot / 180.0 * numpy.pi
    tan_thetax = numpy.tan(thetax)
    
    z3 = z2 * z2 / (z2 + x2 * tan_thetax)
    x3 = x2 * z2 / (z2 + x2 * tan_thetax)
    y3 = y2 * z3 / z2

    
d  =  1200
xl = -d/2
yl = -d/2
pylab.xlim([xl,xl+d])
pylab.ylim([yl,yl+d])
pylab.grid(True)
