// WARNING: logging calls back to InitializeSystemInfo so it must not invoke
// any logging code.
static void InitializeSystemInfo() {
  // If CPU scaling is in effect, we want to use the *maximum* frequency,
  // not whatever CPU speed some random processor happens to be using now.
  bool saw_mhz = false;
  const char* pname0 = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";
  GetUserName();
  FILE* f0 = fopen(pname0, "r");
  if (f0 != NULL) {
    int max_freq;
    if (fscanf(f0, "%d", &max_freq) == 1) {
      // The value is in kHz.  For example, on a 2GHz warpstation, the
      // file contains the value "2000" (with no newline or anything).
      cpuinfo_cycles_per_second = max_freq * 1000.0;
      saw_mhz = true;
    }
    fclose(f0);
  }

  // Read /proc/cpuinfo for other values, and if there is no cpuinfo_max_freq.
  const char* pname = "/proc/cpuinfo";
  FILE* f = fopen(pname, "r");
  CHECK_NE(f, reinterpret_cast<FILE*>(NULL));

  char line[1024];
  double frequency = 1.0;
  double bogo_clock = 1.0;
  int cpu_id;
  int max_cpu_id = 0;
  int sse_count = 0;
  int sse2_count = 0;
  int num_cpus = 0;
  std::stringstream s;
  std::string dummy_str1, dummy_str2, dummy_str3;
  while ( fgets(line, sizeof(line), f) ) {
    line[sizeof(line)-1] = '\0';

    if (!saw_mhz && strstr(line, "cpu MHz") != NULL) {
      s.str("");  // donot forget to clear
      s << line;
      // format is "cpu MHz : %lf"
      s >> dummy_str1 >> dummy_str2 >> dummy_str3 >> frequency;
      cpuinfo_cycles_per_second = frequency * 1000000.0;
      saw_mhz = true;
    } else if (strstr(line, "bogomips") != NULL) {
      s.str("");  // donot forget to clear
      s << line;
      // format is "bogomips : %lf"
      s >> dummy_str1 >> dummy_str2 >> frequency;
      bogo_clock = frequency * 1000000.0;
    } else if (strstr(line, "processor") != NULL) {
      s.str("");  // donot forget to clear
      s << line;
      // format is "processor : %d"
      s >> dummy_str1 >> dummy_str2 >> cpu_id;
      num_cpus++;  // count up every time we see an "processor :" entry
      max_cpu_id = std::max(max_cpu_id, cpu_id);
    }

    // check for sse{2,3}, occurs on the same line
    // verify that all cpus have the same info
    // sse, sse2, sse3 always appear in that order
    // sse2 means sse is also available, sse3 means that
    // sse2 is also available
    const char* sse = strstr(line, " sse");
    const char* sse2 = strstr(line, " sse2");
    const char* sse3 = strstr(line, " sse3");
    if (sse || sse2 || sse3) {
      cpuinfo_support_sse = true;
      sse_count++;
    }
    if (sse2 || sse3) {
      cpuinfo_support_sse2 = true;
      sse2_count++;
    }
  }
  fclose(f);
  if (!saw_mhz) {
    // If we didn't find anything better, we'll use bogomips, but
    // we're not happy about it.
    cpuinfo_cycles_per_second = bogo_clock;
  }
  if ((sse_count > 0) && (sse_count != num_cpus)) {
    fprintf(stderr, "Only %d of %d cpus have SSE, disabling SSE\n",
            sse_count, num_cpus);
    cpuinfo_support_sse = false;
  }
  if ((sse2_count > 0) && (sse2_count != num_cpus)) {
    fprintf(stderr, "Only %d of %d cpus have SSE2, disabing SSE2\n",
            sse2_count, num_cpus);
    cpuinfo_support_sse2 = false;
  }

  if (num_cpus == 0) {
    fprintf(stderr, "Failed to read num. CPUs correctly from /proc/cpuinfo\n");
  } else {
    if ((max_cpu_id + 1) != num_cpus) {
      fprintf(stderr,
              "CPU ID assignments in /proc/cpuinfo seems messed up."
              " This is usually caused by a bad BIOS.\n");
    }
    cpuinfo_num_cpus = num_cpus;
  }
}
