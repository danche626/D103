import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.model_selection import train_test_split, GridSearchCV
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import LSTM, Flatten, Dense, Dropout, Reshape
from tensorflow.keras import Input, Model
from tensorflow.keras.optimizers import Adam
from tensorflow.keras.callbacks import ReduceLROnPlateau
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
    data = pd.read_csv(file_path)
    return data


# 数据预处理
def preprocess_data(data):
    # 处理缺失值
    data = data.ffill()

    # 增加时间特征
    if 'date' in data.columns:
        data['date'] = pd.to_datetime(data['date'])
        data['year'] = data['date'].dt.year
        data['month'] = data['date'].dt.month
        data['day'] = data['date'].dt.day

    # 引入1 - 3阶滞后项
    for i in range(1, 4):
        data[f'sales_lag_{i}'] = data['sales'].shift(i)
    data = data.dropna()

    # 处理异常值
    Q1 = data['sales'].quantile(0.25)
    Q3 = data['sales'].quantile(0.75)
    IQR = Q3 - Q1
    data = data[(data['sales'] >= Q1 - 1.5 * IQR) & (data['sales'] <= Q3 + 1.5 * IQR)]

    # 分离特征和目标变量
    X = data.drop('sales', axis=1)
    y = data['sales']

    # 处理分类变量
    categorical_cols = X.select_dtypes(include=['object']).columns
    encoder = OneHotEncoder(sparse_output=False)
    X_categorical = encoder.fit_transform(X[categorical_cols])

    # 获取数值特征
    X_numerical = X.drop(categorical_cols, axis=1)

    # 如果有数值特征，则进行标准化
    if not X_numerical.empty:
        scaler = StandardScaler()
        X_numerical = scaler.fit_transform(X_numerical)
    else:
        X_numerical = np.empty((X.shape[0], 0))

    # 合并特征
    X = np.hstack([X_numerical, X_categorical])

    return X, y


# 构建LSTM模型
def build_lstm_model(input_shape):
    model = Sequential()
    model.add(LSTM(128, input_shape=input_shape, return_sequences=True))
    model.add(LSTM(64, return_sequences=False))
    model.add(Flatten())
    model.add(Dropout(0.2))
    model.add(Dense(64, activation='relu'))
    model.add(Dropout(0.2))
    model.add(Dense(1))
    model.compile(optimizer='adam', loss='mse')
    return model


# 构建XGBoost模型并使用网格搜索优化
def build_xgboost_model():
    model = xgb.XGBRegressor()
    param_grid = {
        'n_estimators': [200, 300, 400],
        'learning_rate': [0.01, 0.05, 0.1],
        'max_depth': [4, 5, 6],
        'subsample': [0.8, 0.9, 1.0],
        'colsample_bytree': [0.8, 0.9, 1.0]
    }
    grid_search = GridSearchCV(model, param_grid, cv=3, scoring='neg_mean_squared_error')
    return grid_search


# 构建具有时空滞后性的XGBoost模型
def build_xgboost_spatiotemporal_model():
    model = xgb.XGBRegressor()
    param_grid = {
        'n_estimators': [200, 300, 400],
        'learning_rate': [0.01, 0.05, 0.1],
        'max_depth': [4, 5, 6],
        'subsample': [0.8, 0.9, 1.0],
        'colsample_bytree': [0.8, 0.9, 1.0]
    }
    grid_search = GridSearchCV(model, param_grid, cv=3, scoring='neg_mean_squared_error')
    return grid_search


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


# 构建LSTM - Attention模型
def build_lstm_attention_model(input_shape):
    inputs = Input(shape=input_shape)
    # 增加LSTM层
    x = LSTM(128, return_sequences=True)(inputs)
    x = Dropout(0.2)(x)
    x = LSTM(256, return_sequences=True)(x)
    x = Dropout(0.2)(x)
    x = LSTM(512, return_sequences=True)(x)
    x = Dropout(0.2)(x)
    x = LSTM(256, return_sequences=False)(x)
    x = Dropout(0.2)(x)

    # 调整形状以适应多头注意力机制
    sequence_length = input_shape[1]
    features = 256
    reshaped = Reshape((sequence_length, features))(x)

    # 多头注意力机制
    num_heads = 4
    d_model = features
    multihead_attention = MultiHeadAttention(num_heads, d_model)
    attention_output, _ = multihead_attention(reshaped, reshaped, reshaped, mask=None)

    x = Flatten()(attention_output)
    # 增加全连接层
    x = Dense(512, activation='relu')(x)
    x = Dropout(0.2)(x)
    x = Dense(256, activation='relu')(x)
    x = Dropout(0.2)(x)
    x = Dense(128, activation='relu')(x)
    x = Dropout(0.2)(x)
    outputs = Dense(1, activation='linear')(x)

    model = Model(inputs=inputs, outputs=outputs)
    model.compile(optimizer=Adam(learning_rate=0.001), loss='mse')
    return model


# 可视化结果
def visualize_results(y_test, y_pred, title, save_path):
    plt.figure(figsize=(12, 8))
    # 绘制真实值
    plt.plot(y_test.values, label='真实值', color='blue', linewidth=2, linestyle='-', marker='o', markersize=6)
    # 绘制预测值
    plt.plot(y_pred, label='预测值', color='red', linewidth=2, linestyle='--', marker='s', markersize=6)

    # 添加数据标签
    for x, y in enumerate(y_test.values):
        plt.annotate(f'{y:.2f}', (x, y), textcoords='offset points', xytext=(0, 5), ha='center', fontsize=8)
    for x, y in enumerate(y_pred):
        plt.annotate(f'{y:.2f}', (x, y), textcoords='offset points', xytext=(0, -5), ha='center', fontsize=8)

    plt.title(title, fontsize=18)
    plt.xlabel('样本编号', fontsize=14)
    plt.ylabel('销量', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(fontsize=14, loc='upper left')
    plt.tight_layout()
    plt.savefig(save_path)
    plt.close()
    print(f"图表已保存到 {save_path}")


# 主函数
def main():
    # 创建结果文件夹
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    result_dir = f"results_{timestamp}"
    os.makedirs(result_dir, exist_ok=True)

    file_path = '/Users/ruanji/Desktop/统建/数据/华北地区.csv'
    desktop_path = os.path.join(os.path.expanduser("~"), "Desktop")

    best_mse = float('inf')
    best_mape = float('inf')
    best_params = None
    best_y_test = None
    best_y_pred = None

    for i in range(5):  # 修改为5个循环
        print(f"\n正在运行第 {i + 1} 次...")
        start_time = time.time()

        data = read_data(file_path)
        X, y = preprocess_data(data)
        X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42 + i)
        X_train_lstm = X_train.reshape(X_train.shape[0], 1, X_train.shape[1])
        X_test_lstm = X_test.reshape(X_test.shape[0], 1, X_test.shape[1])

        # 训练LSTM模型
        lstm_model = build_lstm_model(input_shape=(1, X_train.shape[1]))
        lstm_model.fit(X_train_lstm, y_train, epochs=20, batch_size=32, verbose=0)
        lstm_features_train = lstm_model.predict(X_train_lstm)
        lstm_features_test = lstm_model.predict(X_test_lstm)
        X_train_combined = np.hstack([X_train, lstm_features_train])
        X_test_combined = np.hstack([X_test, lstm_features_test])

        # 训练XGBoost模型
        xgboost_model = build_xgboost_model()
        xgboost_model.fit(X_train_combined, y_train)
        y_pred_xgboost = xgboost_model.predict(X_test_combined)

        # 训练具有时空滞后性的XGBoost模型
        xgboost_spatiotemporal_model = build_xgboost_spatiotemporal_model()
        xgboost_spatiotemporal_model.fit(X_train_combined, y_train)
        y_pred_xgboost_spatiotemporal = xgboost_spatiotemporal_model.predict(X_test_combined)

        # 训练LSTM - Attention模型
        lstm_attention_model = build_lstm_attention_model(input_shape=(1, X_train.shape[1]))
        # 添加学习率回调
        reduce_lr = ReduceLROnPlateau(monitor='val_loss', factor=0.2, patience=5, min_lr=0.00001)
        try:
            # 增加训练轮数
            lstm_attention_model.fit(X_train_lstm, y_train, epochs=100, batch_size=32,
                                     validation_data=(X_test_lstm, y_test), callbacks=[reduce_lr], verbose=0)
            y_pred_lstm_attention = lstm_attention_model.predict(X_test_lstm).flatten()
        except ValueError as e:
            print(f"训练LSTM - Attention模型时出现错误: {e}")
            continue

        # 评估模型
        mse_xgboost = mean_squared_error(y_test, y_pred_xgboost)
        mape_xgboost = mean_absolute_percentage_error(y_test, y_pred_xgboost)

        mse_xgboost_spatiotemporal = mean_squared_error(y_test, y_pred_xgboost_spatiotemporal)
        mape_xgboost_spatiotemporal = mean_absolute_percentage_error(y_test, y_pred_xgboost_spatiotemporal)

        mse_lstm_attention = mean_squared_error(y_test, y_pred_lstm_attention)
        mape_lstm_attention = mean_absolute_percentage_error(y_test, y_pred_lstm_attention)

        # 保存结果
        result = {
            'run_number': i + 1,
            'xgboost_params': xgboost_model.best_params_,
            'xgboost_spatiotemporal_params': xgboost_spatiotemporal_model.best_params_,
            'mse_xgboost': mse