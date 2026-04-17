from setuptools import find_packages, setup

package_name = 'robot_tools'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='rusanrod',
    maintainer_email='rsandovr@gmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    entry_points={
        'console_scripts': [
            'pnp = robot_tools.pick_place_fsm:main',
            'ejecutor_restaurant = robot_tools.restaurant_executor:main',
        ],
    },
    
)
