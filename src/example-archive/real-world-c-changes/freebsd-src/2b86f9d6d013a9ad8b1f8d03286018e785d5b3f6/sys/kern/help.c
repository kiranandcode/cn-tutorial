struct debug_hashstat_stats {
  int used; int maxlength; int pct;
};

struct debug_hashstat_stats helper() {
  return ((struct debug_hashstat_stats){ .used=0, .maxlength=1, .pct=10 });
}
