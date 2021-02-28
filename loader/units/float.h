// NOTE: this file is replicated from the server side

#ifndef PTPF_FLOAT_H
#define PTPF_FLOAT_H

// NOTE: actual encoding of float on GPU side might be different from the representation on the CPU side
//       PTPF_FLOAT is for representing the float on the CPU side
//       on the GPU side, use..
//       .. the "scalar data type" float for HLSL
//       .. the "the basic non-vector type" float for GLSL
#define PTPF_FLOAT float
#define PTPF_DEFAULT_FLOAT_VALUE 0.0f

#endif
