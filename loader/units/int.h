// NOTE: this file is replicated from the server side

#ifndef PTPF_INT_H
#define PTPF_INT_H

// NOTE: actual encoding of int on GPU side might be different from the representation on the CPU side
//       PTPF_INT is for representing the int on the CPU side
//       on the GPU side, use..
//       .. the "scalar data type" int for HLSL
//       .. the "basic non-vector type" int for GLSL
// NOTE: PTPF does not support unsigned vs. signed integers. If you want to support these in your scripting language..
//       .. I'd recommend you provide this support only for non-bindings (e.g. don't allow writing an unsigned int..
//       .. to a binding without a cast)
#define PTPF_INT int
#define PTPF_DEFAULT_INT_VALUE 0

#endif
