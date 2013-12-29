x=dlmread("minBLEP/data.txt");
oversample=64;
nvalues=size(x)(1)

figure(1)
plot(x, "-b");
hold on;
plot(1:nvalues, ones(1, nvalues), "-r");
plot(1:nvalues, zeros(1, nvalues), "-r");
offset=0
plot(offset+1:oversample:nvalues, x(offset+1:oversample:nvalues), "or");
