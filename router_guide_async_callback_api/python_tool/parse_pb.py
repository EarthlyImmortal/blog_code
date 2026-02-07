import python_pb.route_guide_pb2 as route_guide_pb2

def parse_string_to_pb(input_string):
    """
    将字符串解析为 PB 消息
    
    Args:
        input_string: 二进制文件
    Returns:
        MyMessage: 解析后的 PB 消息对象
    """
    # 创建消息对象
    message = route_guide_pb2.Point()
    
    try:
        message.ParseFromString(input_string)
        return message
    except Exception as e:
        print(f"解析错误: {e}")
        return None

def serialize_to_string(message):
    """将 PB 消息序列化为二进制字符串"""
    return message.SerializeToString()

def display_message_info(message):
    """显示消息内容"""
    if message:
        print("=== PB 消息内容 ===")
        print(f"latitude: {message.latitude}")
        print(f"longitude: {message.longitude}")
        print("===================")

# 示例使用
if __name__ == "__main__":
    # 示例1: 从二进制字符串解析
    print("示例1: 二进制字符串解析")
    
    # 创建一个示例消息并序列化为二进制
    sample_message = route_guide_pb2.Point()
    sample_message.latitude = 414777405
    sample_message.longitude = -740615601
    
    # 序列化为二进制字符串
    binary_data = serialize_to_string(sample_message)
    print(f"二进制数据长度: {len(binary_data)} 字节")

    # 将二进制数据转换为16进制表示
    hex_data = binary_data.hex()  # 方法1：使用bytes.hex()方法
    # 或者 hex_data = binascii.hexlify(binary_data).decode('utf-8')  # 方法2
    print(f"16进制表示: {hex_data}")
    
    # 从二进制字符串解析
    parsed_message = parse_string_to_pb(binary_data)
    display_message_info(parsed_message)