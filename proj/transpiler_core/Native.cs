using System;
using System.Linq;
using System.Reflection.Emit;

namespace transpiler
{
	internal static unsafe class Memory
	{
		public delegate void MemCpyFunction(void* des, void* src, uint bytes);
		public static readonly MemCpyFunction Cpy;

		public delegate void MemSetFunction(void* des, byte value, uint bytes);
		public static readonly MemSetFunction Set;

		static T GenerateMethod<T>(params OpCode[] opCodes) where T : Delegate
		{
			var method = typeof(T).GetMethod("Invoke");
			var dynamicMethod = new DynamicMethod(
				"Cpy",
				method.ReturnType,
				method.GetParameters().Select(t => t.ParameterType).ToArray(),
				typeof(Memory)
			);

			var ilGenerator = dynamicMethod.GetILGenerator();

			foreach (var code in opCodes)
				ilGenerator.Emit(code);

			return (T)dynamicMethod.CreateDelegate(typeof(T));
		}

		static Memory()
		{
			Cpy = GenerateMethod<MemCpyFunction>(OpCodes.Ldarg_0, OpCodes.Ldarg_1, OpCodes.Ldarg_2, OpCodes.Cpblk, OpCodes.Ret);
			Set = GenerateMethod<MemSetFunction>(OpCodes.Ldarg_0, OpCodes.Ldarg_1, OpCodes.Ldarg_2, OpCodes.Initblk, OpCodes.Ret);
		}
	}
}
