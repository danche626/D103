import numpy as np
import pandas as pd

np.random.seed(42)  
n_samples = 1000  
sales_data = pd.DataFrame({  
    'date': pd.date_range(start='2015-01-01', periods=n_samples, freq='M'),  
    'region': np.random.choice(['Region1', 'Region2', 'Region3'], n_samples),  # 确保此行执行  
    'model': np.random.choice(['ModelA', 'ModelB', 'ModelC'], n_samples),  
    'sales': np.random.randint(100, 1000, n_samples),  
    # 其他列...  
})  
print("数据列名：", sales_data.columns)  # 新增：打印列名，检查是否包含 'region'  
