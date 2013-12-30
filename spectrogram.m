# Requires extra packages:
# apt-get install octave-signal

fid=fopen("testdata.bin", "r");
x=fread(fid, Inf, "float");
Fs=44100;
t=1/Fs*(0:(size(x)(1)-1));

figure(1);
step=ceil(20/1000*Fs);    # one spectral slice every 20 ms
window=ceil(40/1000*Fs); # 100 ms data window
specgram(x, 2^nextpow2(window), Fs, window, window-step);

figure(2);
xlabel("time [s]");
plot(t, x);

figure(3);
fftstart=400/1000*Fs;
fftlen=4096;
xlabel("Frequency [Hz]");
freqs=0:Fs/fftlen:Fs-1;
f=fft(x(fftstart:fftstart+fftlen), fftlen);
semilogy(freqs(1:fftlen/2), abs(f)(1:fftlen/2));
axis([0 Fs/2])
