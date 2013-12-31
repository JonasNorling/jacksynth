x=dlmread("minBLEP/data.txt");
oversample=32;
nvalues=size(x)(1)

figure(1)
hold on;
xlabel("time");
plot(1:nvalues, ones(1, nvalues), "-r");
plot(1:nvalues, zeros(1, nvalues), "-r");
plot(x, "-b");
offset=0
plot(offset+1:oversample:nvalues, x(offset+1:oversample:nvalues), "or");

#figure(2)
#fftlen=1024;
#f=fft(x, fftlen);
#xlabel("fft bin")
#freqs=0:1:fftlen-1;
#semilogy(freqs, abs(f));
#axis([0 fftlen/2-1])
