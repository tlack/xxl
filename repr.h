char* repr_b(VP x,char* s,size_t sz) { int i; 
for(i=0;i<x->n;i++) snprintf(s+strlen(s),sz-strlen(s)-1,"%d,",AS_b(x,i)); 
return s; }

char* repr_i(VP x,char* s,size_t sz) { int i; 
for(i=0;i<x->n;i++) snprintf(s+strlen(s),sz-strlen(s)-1,"%d,",AS_i(x,i)); 
return s; }

char* repr_j(VP x,char* s,size_t sz) { int i; 
for(i=0;i<x->n;i++) snprintf(s+strlen(s),sz-strlen(s)-1,"%ld,",AS_j(x,i)); 
return s; }

char* repr_o(VP x,char* s,size_t sz) { int i; 
for(i=0;i<x->n;i++) snprintf(s+strlen(s),sz-strlen(s)-1,"%lld,",AS_o(x,i)); 
return s; }

char* repr_c(VP x,char* s,size_t sz) { int i; 
for(i=0;i<x->n;i++) snprintf(s+strlen(s),sz-strlen(s)-1,"%c,",AS_c(x,i)); 
return s; }

char* repr_1(VP x,char* s,size_t sz) { int i; 
for(i=0;i<x->n;i++) snprintf(s+strlen(s),sz-strlen(s)-1,"%p,",AS_1(x,i)); 
return s; }

char* repr_2(VP x,char* s,size_t sz) { int i; 
for(i=0;i<x->n;i++) snprintf(s+strlen(s),sz-strlen(s)-1,"%p,",AS_2(x,i)); 
return s; }

char* repr_p(VP x,char* s,size_t sz) { int i; 
for(i=0;i<x->n;i++) snprintf(s+strlen(s),sz-strlen(s)-1,"%p,",AS_p(x,i)); 
return s; }

