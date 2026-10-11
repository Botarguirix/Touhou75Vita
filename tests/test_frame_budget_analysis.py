import importlib.util
from pathlib import Path
import unittest

spec=importlib.util.spec_from_file_location('budget',Path(__file__).parents[1]/'tools/analyze_frame_budget.py')
budget=importlib.util.module_from_spec(spec)
spec.loader.exec_module(budget)


def cycle(us,draw=0,present=0,presents=1):
    return ('startup_frame_wait=blocked frame:1\n'
            f'startup_d3d8_draw_elapsed_us={draw} call:1\n'
            f'startup_d3d8_present_elapsed_us={present} frame:1\n'+
            'startup_d3d8_present=executed frame:1\n'*presents+
            f'startup_frame_cycle_elapsed_us={us} cycle:1\n')


class BudgetAnalysis(unittest.TestCase):
    def test_empty(self):
        result=budget.analyze('startup_d3d8_present=executed frame:1\n')
        self.assertIsNone(result['all']['fps'])
        self.assertEqual(result['all']['frames'],0)

    def test_scope_and_deadline(self):
        result=budget.analyze(cycle(16667,5000,6000)+cycle(16668,5000,6000))
        self.assertEqual(result['all']['frames'],2)
        self.assertEqual(result['all']['within_60fps_budget'],1)
        self.assertEqual(result['all']['missed_60fps_budget'],1)
        self.assertEqual(result['all']['remaining_mean_us'],5667.5)

    def test_exclusion(self):
        result=budget.analyze(cycle(1000,presents=0)+cycle(1000,presents=2)+cycle(0))
        self.assertEqual(result['all']['frames'],0)
        self.assertEqual(result['excluded_no_multi_present_or_invalid'],3)
        self.assertEqual(budget.analyze('startup_frame_wait=blocked frame:1\n')['incomplete_cycles'],1)

    def test_menu_and_quantile(self):
        data='startup_frame_scene=object:0x00A00000 vtable:0x00658308 update:0x1\n'
        result=budget.analyze(data+''.join(cycle(n) for n in range(100,0,-1)))
        self.assertEqual(result['menu']['frames'],100)
        self.assertEqual(result['all']['p50_us'],50)
        self.assertEqual(result['all']['p95_us'],95)
        self.assertEqual(result['all']['p99_us'],99)

    def test_overlap_and_initialization(self):
        result=budget.analyze('startup_d3d8_draw_elapsed_us=999999 call:1\n'+cycle(1000,600,600))
        self.assertEqual(result['all']['draw_mean_us'],600)
        self.assertEqual(result['all']['timing_overlap_cycles'],1)
        self.assertEqual(result['all']['remaining_mean_us'],0)

    def test_scene_groups_and_long_session_bound(self):
        data='startup_frame_scene=object:0x00C00000 vtable:0x00658308\n'+cycle(10000)
        data+='startup_frame_scene=object:0x00C10000 vtable:0x00658400\n'+cycle(330000000)
        result=budget.analyze(data)
        self.assertEqual(result['menu']['frames'],1)
        self.assertEqual(result['scenes']['0X00658400']['frames'],1)
        self.assertEqual(result['all']['frames'],2)
        self.assertEqual(budget.analyze(cycle(330000001))['all']['frames'],0)


if __name__=='__main__':
    unittest.main()
