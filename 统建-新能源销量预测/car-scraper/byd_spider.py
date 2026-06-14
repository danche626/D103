import requests
from bs4 import BeautifulSoup
from datetime import datetime
import pandas as pd


def get_byd_sales():
    try:
        url = 'https://xl.16888.com/f/57329/'
        headers = {
            'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.114 Safari/537.36'
        }
        response = requests.get(url, headers=headers)
        response.raise_for_status()

        soup = BeautifulSoup(response.text, 'html.parser')
        # 根据实际网页结构修改选择器，定位表格的行
        rows = soup.select('.xl-table-def.xl-table-a tr')[1:]  # 跳过表头
        sales_data = []
        for row in rows:
            # 根据实际网页结构修改选择器，定位日期和销量单元格
            date_cell = row.select_one('.xl-td-t4:nth-of-type(1)')
            sales_cell = row.select_one('.xl-td-t4:nth-of-type(2)')
            if date_cell and sales_cell:
                date_str = date_cell.text.strip()
                sales = sales_cell.text.strip()
                try:
                    # 修改日期格式
                    date = datetime.strptime(date_str, '%Y-%m')
                    if datetime(2021, 2, 1) <= date <= datetime(2025, 2, 28):
                        sales_data.append((date_str, sales))
                except ValueError:
                    continue
        return sales_data
    except requests.RequestException as e:
        print(f"请求出错: {e}")
    except AttributeError:
        print("未找到销量数据，请检查网页结构和选择器。")
    return []


if __name__ == "__main__":
    sales = get_byd_sales()
    if sales:
        print("2021 年 2 月到 2025 年 2 月的 BYD 汽车销量数据：")
        for date, sale in sales:
            print(f"{date}: {sale}")
        # 将数据转换为 DataFrame
        df = pd.DataFrame(sales, columns=['日期', '销量'])
        # 保存为 CSV 文件
        df.to_csv('byd_sales.csv', index=False, encoding='utf-8-sig')
        print("数据已成功保存为 byd_sales.csv 文件。")
    else:
        print("未获取到有效数据。")
    