import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler, MinMaxScaler, OneHotEncoder
from sklearn.model_selection import train_test_split, GridSearchCV
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Conv1D, MaxPooling1D, LSTM, Flatten, Dense, Dropout, Reshape, Input, ConvLSTM2D, GlobalAveragePooling1D, BatchNormalization
from tensorflow.keras import Model
from tensorflow.keras.optimizers import Adam
from tensorflow.keras.callbacks import ReduceLROnPlateau, EarlyStopping, LearningRateScheduler
import tensorflow as tf
import xgboost as xgb
from sklearn.metrics import mean_squared_error, mean_absolute_percentage_error
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import os
import time
from datetime import datetime


# 尝试直接指定字体文件
font_path = '/System/Library/AssetsV2/com_apple_MobileAsset_Font7/3419f2a427639ad8c8e139149a287865a90fa17e.asset/AssetData/PingFang.ttc'
if os.path.exists(font_path):
    font_prop = fm.FontProperties(fname=font_path)
    plt.rcParams['font.family'] = font_prop.get_name()
    plt.rcParams['axes.unicode_minus'] = False
    print("成功使用字体: PingFang SC")
else:
    print("警告：未找到PingFang字体，使用默认字体")


# 读取CSV数据
def read_data(file_path):
    return pd.read_csv(file_path)


# 数据预处理
def preprocess_data(data):
    # 更严格的异常值处理
    if 'sales' in data.columns:
        Q1 = data['sales'].quantile(0.25)
        Q3 = data['sales'].quantile(0.75)
        IQR = Q3 - Q1
        data = data[(data['sales'] >= Q1 - 1.5 * IQR) & (data['sales'] <= Q3 + 1.5 * IQR)]

        data.loc[data['sales'] > 1e5, 'sales'] = np.nan
        # 将 date 列设为索引
        if 'date' in data.columns:
            data = data.set_index('date')
        data['sales'] = data['sales'].interpolate(method='time')  # 时间序列插值

        # 使用移动标准差检测异常值
        rolling_std = data['sales'].rolling(window=6).std()
        data = data[(data['sales'] <= data['sales'].mean() + 3*rolling_std) &
                    (data['sales'] >= data['sales'].mean() - 3*rolling_std)]

    # 扩展滞后特征到12个月
    if 'sales' in data.columns:
        for lag in range(1, 13):
            data[f'lag_{lag}'] = data['sales'].shift(lag)

    # 添加周期性特征
    if 'date' in data.index.names:
        data['month'] = data.index.month
        data['month_sin'] = np.sin(2 * np.pi * data['month']/12)
        data['month_cos'] = np.cos(2 * np.pi * data['month']/12)

    # 添加节假日信息（假设存在一个包含节假日信息的 CSV 文件）
    try:
        holidays = pd.read_csv('holidays.csv')
        data = pd.merge(data.reset_index(), holidays, on='date', how='left')
        data['is_holiday'] = data['holiday'].notnull().astype(int)
        data = data.set_index('date')
    except FileNotFoundError:
        print("警告：未找到 holidays.csv 文件，未添加节假日信息。")

    return data.dropna().reset_index()  # 重置索引


# 构建ConvLSTM模型
def build_convlstm_model(input_shape):
    pass


# 构建XGBoost模型并使用网格搜索优化
def build_xgboost_model():
    pass


# 构建具有时空滞后性的XGBoost模型
def build_xgboost_spatiotemporal_model():
    pass


# 多头注意力机制
class MultiHeadAttention(tf.keras.layers.Layer):
    def __init__(self, num_heads, d_model):
        super(MultiHeadAttention, self).__init__()
        self.num_heads = num_heads
        self.d_model = d_model

        assert d_model % self.num_heads == 0

        self.depth = d_model // self.num_heads

        self.wq = Dense(d_model)
        self.wk = Dense(d_model)
        self.wv = Dense(d_model)

        self.dense = Dense(d_model)

    def split_heads(self, x, batch_size):
        """Split the last dimension into (num_heads, depth).
        Transpose the result such that the shape is (batch_size, num_heads, seq_len, depth)
        """
        x = tf.reshape(x, (batch_size, -1, self.num_heads, self.depth))
        return tf.transpose(x, perm=[0, 2, 1, 3])

    def call(self, v, k, q, mask):
        batch_size = tf.shape(q)[0]

        q = self.wq(q)  # (batch_size, seq_len, d_model)
        k = self.wk(k)  # (batch_size, seq_len, d_model)
        v = self.wv(v)  # (batch_size, seq_len, d_model)

        q = self.split_heads(q, batch_size)  # (batch_size, num_heads, seq_len_q, depth)
        k = self.split_heads(k, batch_size)  # (batch_size, num_heads, seq_len_k, depth)
        v = self.split_heads(v, batch_size)  # (batch_size, num_heads, seq_len_v, depth)

        # scaled_attention.shape == (batch_size, num_heads, seq_len_q, depth)
        # attention_weights.shape == (batch_size, num_heads, seq_len_q, seq_len_k)
        scaled_attention, attention_weights = self.scaled_dot_product_attention(
            q, k, v, mask)

        scaled_attention = tf.transpose(scaled_attention, perm=[0, 2, 1, 3])  # (batch_size, seq_len_q, num_heads, depth)

        concat_attention = tf.reshape(scaled_attention,
                                      (batch_size, -1, self.d_model))  # (batch_size, seq_len_q, d_model)

        output = self.dense(concat_attention)  # (batch_size, seq_len_q, d_model)

        return output, attention_weights

    def scaled_dot_product_attention(self, q, k, v, mask):
        """Calculate the attention weights.
        q, k, v must have matching leading dimensions.
        k, v must have matching penultimate dimension, i.e.: seq_len_k = seq_len_v.
        The mask has different shapes depending on its type(padding or look ahead)
        but it must be broadcastable for addition.

        Args:
          q: query shape == (..., seq_len_q, depth)
          k: key shape == (..., seq_len_k, depth)
          v: value shape == (..., seq_len_v, depth_v)
          mask: Float tensor with shape broadcastable
                to (..., seq_len_q, seq_len_k). Defaults to None.

        Returns:
          output, attention_weights
        """

        matmul_qk = tf.matmul(q, k, transpose_b=True)  # (..., seq_len_q, seq_len_k)

        # scale matmul_qk
        dk = tf.cast(tf.shape(k)[-1], tf.float32)
        scaled_attention_logits = matmul_qk / tf.math.sqrt(dk)

        # add the mask to the scaled tensor.
        if mask is not None:
            scaled_attention_logits += (mask * -1e9)

        # softmax is normalized on the last axis (seq_len_k) so that the scores
        # add up to 1.
        attention_weights = tf.nn.softmax(scaled_attention_logits, axis=-1)  # (..., seq_len_q, seq_len_k)

        output = tf.matmul(attention_weights, v)  # (..., seq_len_q, depth_v)

        return output, attention_weights


# 构建CNN - LSTM - Attention模型
def build_cnn_lstm_attention_model(input_shape):
    inputs = Input(shape=input_shape)

    # 特征提取
    x = Conv1D(128, 3, activation='relu', padding='same')(inputs)
    x = BatchNormalization()(x)  # 添加批量归一化层
    print("Conv1D_1 output shape:", x.shape)
    x = MaxPooling1D(2)(x)
    print("MaxPooling1D_1 output shape:", x.shape)
    x = Conv1D(256, 3, activation='relu', padding='same')(x)  # 增加卷积层
    x = BatchNormalization()(x)  # 添加批量归一化层
    print("Conv1D_2 output shape:", x.shape)
    x = MaxPooling1D(2)(x)
    print("MaxPooling1D_2 output shape:", x.shape)
    x = LSTM(256, return_sequences=True, dropout=0.1, recurrent_dropout=0.1)(x)  # 添加 dropout
    print("LSTM output shape:", x.shape)

    # 多头注意力机制
    multihead_attention = MultiHeadAttention(num_heads=4, d_model=256)
    x, _ = multihead_attention(x, x, x, mask=None)
    print("MultiHeadAttention output shape:", x.shape)

    # 使用 GlobalAveragePooling1D 替代 Flatten
    x = GlobalAveragePooling1D()(x)
    print("GlobalAveragePooling1D output shape:", x.shape)

    x = Dense(256, activation='relu')(x)
    x = BatchNormalization()(x)  # 添加批量归一化层
    print("Dense_1 output shape:", x.shape)
    x = Dropout(0.2)(x)  # 添加 Dropout 层防止过拟合
    x = Dense(128, activation='relu')(x)
    x = BatchNormalization()(x)  # 添加批量归一化层
    print("Dense_2 output shape:", x.shape)
    x = Dropout(0.2)(x)
    outputs = Dense(1)(x)

    model = Model(inputs=inputs, outputs=outputs)
    model.compile(optimizer=Adam(learning_rate=0.0005), loss='mse')  # 降低学习率
    return model


# 可视化结果
def visualize_results(y_test, y_pred, title, save_path):
    plt.figure(figsize=(10, 6))
    plt.plot(range(len(y_test)), y_test.values, label='真实值', color='#1f77b4', marker='o')
    plt.plot(range(len(y_pred)), y_pred, label='预测值', color='#ff7f0e', linestyle='--', marker='s')

    # 标注数据点
    for i, (y_true, y_pred_val) in enumerate(zip(y_test.values, y_pred)):
        plt.annotate(f'{y_true:.2f}', (i, y_true), textcoords='offset points', xytext=(0, 5), ha='center', fontsize=8)
        plt.annotate(f'{y_pred_val:.2f}', (i, y_pred_val), textcoords='offset points', xytext=(0, -5), ha='center', fontsize=8)

    plt.title(title, fontsize=16)
    plt.xlabel('样本编号', fontsize=14)
    plt.ylabel('销量', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(fontsize=14)
    plt.tight_layout()
    plt.savefig(save_path, dpi=300)


# 学习率调度器
def lr_scheduler(epoch, lr):
    if epoch < 50:
        return float(lr)
    else:
        return float(lr * tf.math.exp(-0.1))


# 主函数
def main():
    # 创建结果文件夹
    if not os.path.exists('results'):
        os.makedirs('results')

    # 读取数据
    data = read_data('/Users/ruanji/Desktop/统建/数据/华北地区.csv')
    # 检查 'date' 列是否存在
    if 'date' not in data.columns:
        print("警告：数据文件中不存在 'date' 列，无法添加周期性特征。")
    else:
        # 明确指定日期格式
        data['date'] = pd.to_datetime(data['date'], format='%Y_%m')

    # 数据预处理
    processed_data = preprocess_data(data)

    # 划分特征和目标变量
    if 'date' in processed_data.columns:
        X = processed_data.drop(['date', 'sales'], axis=1)
    else:
        X = processed_data.drop(['sales'], axis=1)
    y = processed_data['sales']

    # 划分训练集和测试集
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

    # 数据标准化（使用 MinMaxScaler）
    scaler = MinMaxScaler()
    X_train = scaler.fit_transform(X_train)
    X_test = scaler.transform(X_test)

    # 调整输入形状
    X_train_cnn_lstm = np.expand_dims(X_train, axis=2)
    X_test_cnn_lstm = np.expand_dims(X_test, axis=2)

    # 构建模型
    input_shape = X_train_cnn_lstm.shape[1:]
    model = build_cnn_lstm_attention_model(input_shape)
    model.summary()

    # 训练模型
    history = model.fit(
        X_train_cnn_lstm, y_train,
        epochs=800,  # 增加训练轮数
        batch_size=32,  # 调整批量大小
        validation_split=0.2,
        callbacks=[
            EarlyStopping(patience=30),  # 调整早停耐心值
            ReduceLROnPlateau(factor=0.2, patience=10),  # 调整学习率衰减因子和耐心值
            LearningRateScheduler(lr_scheduler)
        ],
        verbose=1
    )

    # 训练历史可视化
    plt.figure()
    plt.plot(history.history['loss'], label='训练损失')
    plt.plot(history.history['val_loss'], label='验证损失')
    plt.legend()
    plt.savefig('results/training_history.png', dpi=300, bbox_inches='tight')

    # 预测
    y_pred = model.predict(X_test_cnn_lstm).flatten()

    # 打印预测值范围
    print(f"预测值范围: {np.min(y_pred)} - {np.max(y_pred)}")
    print(f"真实值范围: {np.min(y_test)} - {np.max(y_test)}")

    # 可视化结果
    visualize_results(y_test, y_pred, 'CNN-LSTM-Attention模型预测结果', 'results/prediction_results.png')


if __name__ == "__main__":
    main()
    