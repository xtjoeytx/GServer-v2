using System.Runtime.InteropServices;

namespace Gs2Emu;

public delegate void StringPrintDelegate(string data);

public static class Global
{
	[DllImport("gs2emu_d", CallingConvention = CallingConvention.Cdecl)]
	private static extern void register_log_callback(IntPtr callback);

	public static void RegisterLogCallback(StringPrintDelegate delegateVar) => register_log_callback(Marshal.GetFunctionPointerForDelegate(delegateVar));
}

public class Server(string? name)
{
	private readonly IntPtr _server = get_server_instance(name);

	[DllImport("gs2emu_d", CallingConvention = CallingConvention.Cdecl)]
	private static extern IntPtr get_server_instance(string? serverName);

	[DllImport("gs2emu_d", CallingConvention = CallingConvention.Cdecl)]
	private static extern int initialize_server_instance(IntPtr context, string? overrideServerIp, string? overridePort, string? overrideLocalIp, string? overrideServerInterface, string? overrideName, string? overrideStaff, string? overrideListIp, string? overrideListPort);

	[DllImport("gs2emu_d", CallingConvention = CallingConvention.Cdecl)]
	private static extern void start_server_instance(IntPtr context);


	[DllImport("gs2emu_d", CallingConvention = CallingConvention.Cdecl)]
	private static extern void shutdown_server_instance(IntPtr context);

	~Server()
	{
		shutdown_server_instance(_server);
	}

	public int Init(
		string? overrideServerIp = null,
		string? overridePort = null,
		string? overrideLocalIp = null,
		string? overrideServerInterface = null,
		string? overrideName = null,
		string? overrideStaff = null,
		string? overrideListIp = null,
		string? overrideListPort = null
	)
	{
		if (_server == IntPtr.Zero) return 1;
		return initialize_server_instance(
			_server,
			overrideServerIp,
			overridePort,
			overrideLocalIp,
			overrideServerInterface,
			overrideName,
			overrideStaff,
			overrideListIp,
			overrideListPort
		);
	}

	public void Start()
	{
		if (_server == IntPtr.Zero) return;
		start_server_instance(_server);
	}
}