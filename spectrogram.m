# Requires extra packages:
# apt-get install octave-signal

fid=fopen(filename, "r");
x=fread(fid, Inf, "float");
Fs=44100;
t=1/Fs*(0:(size(x)(1)-1));

figure(1);
step=ceil(5/1000*Fs);    # one spectral slice every 5 ms
window=ceil(20/1000*Fs); # 20 ms data window
specgram(x, 2^nextpow2(window), Fs, window, window-step);

figure(2);
xlabel("time [s]");
plot(t, x, "-b");
hold on;
[b,a]=butter(3, 0.0005);
plot(t, filter(b, a, x), "-r")
hold off;

figure(3);
fftstart=5200/1000*Fs; # Spectrum from 5.2s
fftlen=2048;
xlabel("Frequency [Hz]");
freqs=0:Fs/fftlen:Fs-1;
blackman_fft=fft(blackman(fftlen) .* x(fftstart:fftstart+fftlen-1) ./ fftlen, fftlen);
rect_fft=fft(x(fftstart:fftstart+fftlen-1) ./ fftlen, fftlen);
semilogy(freqs(1:fftlen/2), abs(rect_fft)(1:fftlen/2) .^ 2, "-r");
hold on;
semilogy(freqs(1:fftlen/2), abs(blackman_fft)(1:fftlen/2) .^ 2, "-b");
axis([0 Fs/2, 1e-16, 1e-1])
hold off;
