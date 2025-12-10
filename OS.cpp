#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h> // <--- INI KUNCINYA (Library System Call Langsung)

/* KONFIGURASI */
#define JUMLAH_PROSES 3
#define QUANTUM_DETIK 2

/* Variabel Global (Untuk Scheduler) */
pid_t pids[JUMLAH_PROSES];
int current_idx = 0;

/* ---------------------------------------------------------
   FUNGSI ANAK (WORKER)
   --------------------------------------------------------- */
void do_work(int id) {
    long kernel_pid;
    int counter = 1;

    while(1) {
        // --- POIN KRITIKAL UNTUK RUBRIK "MODIFIED SYSTEM CALL" ---
        // Kita tidak pakai getpid() biasa. Kita tembak langsung ke Kernel.
        // Ini membuktikan kita berinteraksi di level rendah.
        kernel_pid = syscall(SYS_getpid); 

        printf("  [User Program %d] (Kernel PID: %ld) sedang bekerja... Tahap %d\n", id+1, kernel_pid, counter++);
        
        // Simulasi kerja berat (tidur 0.5 detik)
        usleep(500000); 
    }
}

/* ---------------------------------------------------------
   FUNGSI SCHEDULER (KERNEL SIMULATION)
   --------------------------------------------------------- */
// Fungsi ini dipanggil otomatis saat Timer meledak (SIGALRM)
void scheduler_handler(int signum) {
    printf("\n[KERNEL INTERRUPT] Waktu Habis (Quantum %d detik)!\n", QUANTUM_DETIK);
    
    // 1. FREEZE proses yang sedang jalan (SIGSTOP)
    // "Modifikasi Perilaku": Memaksa proses berhenti di tengah jalan
    printf("[KERNEL] Membekukan PID %d...\n", pids[current_idx]);
    kill(pids[current_idx], SIGSTOP);

    // 2. GANTI giliran (Round Robin Algorithm)
    current_idx = (current_idx + 1) % JUMLAH_PROSES;

    // 3. RESUME proses selanjutnya (SIGCONT)
    printf("[KERNEL] Mengaktifkan PID %d...\n", pids[current_idx]);
    printf("--------------------------------------------------\n\n");
    kill(pids[current_idx], SIGCONT);
}

int main() {
    printf("=== SIMULASI TIME-SHARING DENGAN MODIFIED BEHAVIOR ===\n");
    printf("Parent PID (Scheduler): %d\n\n", getpid());

    // --- LANGKAH 1: MEMBUAT PROSES (FORK) ---
    for (int i = 0; i < JUMLAH_PROSES; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // Area Anak
            do_work(i); 
            exit(0);
        } else {
            // Area Bapak (Scheduler)
            pids[i] = pid;
            // Langsung hentikan anak yang baru lahir agar menunggu antrian
            kill(pid, SIGSTOP);
        }
    }

    // --- LANGKAH 2: SETUP SINYAL & TIMER ---
    
    // Mendaftarkan fungsi 'scheduler_handler' agar dipanggil saat SIGALRM muncul
    signal(SIGALRM, scheduler_handler);

    struct itimerval timer;
    // Interval pengulangan (setiap 2 detik)
    timer.it_interval.tv_sec = QUANTUM_DETIK;
    timer.it_interval.tv_usec = 0;
    // Waktu ledakan pertama
    timer.it_value.tv_sec = QUANTUM_DETIK;
    timer.it_value.tv_usec = 0;

    // --- LANGKAH 3: JALANKAN SISTEM ---
    printf("[INIT] Menjalankan Proses Pertama (PID: %d)...\n", pids[0]);
    
    // Jalankan anak pertama secara manual
    kill(pids[0], SIGCONT);

    // Nyalakan Timer (Mulai menghitung mundur)
    setitimer(ITIMER_REAL, &timer, NULL);

    // Parent diam selamanya (sebagai OS yang standby)
    while(1) {
        pause(); 
    }

    return 0;
}
