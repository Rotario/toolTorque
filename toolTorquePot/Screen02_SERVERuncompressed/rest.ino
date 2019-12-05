/*template <typename T>
struct Variable {
  virtual void addToBuffer() const = 0;
};


template<typename T>
struct TypedVariable: Variable {
  T *var;
  bool quotable;

  TypedVariable(T *v, bool q) : var{v} { quotable = q; }

  void addToBuffer() const override { 
    addToBuffer(*var, quotable);
  }  
};

uint8_t variable_index;
Variable* variables[10];

const char * variable_names[10];

template <typename T> struct PrintfFormat;

template <> struct PrintfFormat<int>
{
   static char const* get() { return "%d"; }
};

template <> struct PrintfFormat<float>
{
   static char const* get() { return "%f"; }
};

template <> struct PrintfFormat<double>
{
   static char const* get() { return "%f"; }
};

template <typename T>
char const* getPrintfFormat()
{
   return PrintfFormat<T>::get();
}*/

uint8_t variable_index = 0;
void * variables[10];
const char * variable_names[10];

void addRestVariable(const char * var, void* ptr){
  if (!(variable_index < 10)){
    variable_index = 0;
  }
  variable_names[variable_index] = var;
  variables[variable_index] = ptr;
  variable_index++;
}
